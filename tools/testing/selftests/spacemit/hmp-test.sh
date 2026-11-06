#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# Spacemit HMP on-target test, BusyBox ash compatible.
# Verified on K3 (x100 = cpu0-7, a100 = cpu8-15).
#
# Usage:
#   ./hmp-test.sh all         # env+baseline+bind+restrict (safe)
#   ./hmp-test.sh hotplug     # cpu offline restrictions (DISRUPTIVE)
# One case a time:
#   ./hmp-test.sh env         # environment / cpumask sanity
#   ./hmp-test.sh baseline    # every task defaults to regular cpus
#   ./hmp-test.sh bind        # bind a task to AI cpus, verify it runs there
#   ./hmp-test.sh restrict    # user affinity to the wrong class is rejected
#
# Override the cpu layout if it differs:
#   REG_CPUS=0-7 AI_CPUS=8-15 ./hmp-test.sh all
#
# Exit status: 0 passed, 1 failed, 2 bad test name, 4 HMP not available
# (CONFIG_SPACEMIT_HMP off, so /proc/set_ai_thread is absent).

REG_CPUS=${REG_CPUS:-0-7}
AI_CPUS=${AI_CPUS:-8-15}

PASS=0
FAIL=0
SKIP=0

# ---------------------------------------------------------------- helpers --

say()  { printf '%s\n' "$*"; }
pass() { PASS=$((PASS + 1)); say "  [PASS] $*"; }
fail() { FAIL=$((FAIL + 1)); say "  [FAIL] $*"; }
skip() { SKIP=$((SKIP + 1)); say "  [SKIP] $*"; }
hdr()  { say ""; say "=== $* ==="; }

# expand a kernel cpu list ("0-7,9,11-13") into one cpu per line
expand_list() {
	echo "$1" | awk -F, '{
		for (i = 1; i <= NF; i++) {
			n = split($i, r, "-");
			if (n < 2) r[2] = r[1];
			for (c = r[1]; c <= r[2]; c++) print c;
		}
	}'
}

# kernel cpu list -> hex cpumask for taskset
cpus_to_mask() {
	expand_list "$1" | awk '{m = m + 2 ^ $1} END {printf "%x\n", m}'
}

# 1 if the two cpu lists share at least one cpu
intersects() {
	for a in $(expand_list "$1" | sort -u); do
		for b in $(expand_list "$2" | sort -u); do
			[ "$a" = "$b" ] && return 0
		done
	done
	return 1
}

# number of cpus in a list
list_weight() {
	expand_list "$1" | wc -l | tr -d ' '
}

# Cpus_allowed_list of a pid
mask_of() {
	grep Cpus_allowed_list /proc/"$1"/status 2>/dev/null | cut -f2 | tr -d ' \t'
}

# cpu a pid is currently running on (field 39 of /proc/pid/stat).
# The comm field may contain spaces, so strip through the last ')' first.
cpu_of() {
	sed 's/.*) //' /proc/"$1"/stat 2>/dev/null | awk '{print $37}'
}

# state a pid is in (field 3; becomes field 1 after the strip above)
state_of() {
	sed 's/.*) //' /proc/"$1"/stat 2>/dev/null | awk '{print $1}'
}

# pids that could actually block the last AI cpu offline.
# hmp_cpu_can_offline() looks at thread_type == HMP_AI, which is not
# user-visible; the observable proxy is a multi-cpu mask on AI cpus.
# Single-cpu masks are per-cpu kthreads (cpuhp/migration/ksoftirqd/
# kworker/N:*), which the kernel allows on AI cpus by design.
pids_multi_on_ai() {
	for p in /proc/[0-9]*; do
		pid=${p#/proc/}
		[ -r "$p/status" ] || continue
		m=$(mask_of "$pid")
		[ -n "$m" ] || continue
		[ "$(list_weight "$m")" -gt 1 ] || continue
		intersects "$m" "$AI_CPUS" && echo "$pid"
	done
}

# ------------------------------------------------------------- test: env --

test_env() {
	hdr "environment"

	say "online cpus : $(cat /sys/devices/system/cpu/online 2>/dev/null)"
	say "offline cpus: $(cat /sys/devices/system/cpu/offline 2>/dev/null)"
	say "reg cpus    : $REG_CPUS"
	say "ai  cpus    : $AI_CPUS"
	say "reg mask    : 0x$(cpus_to_mask "$REG_CPUS")"
	say "ai  mask    : 0x$(cpus_to_mask "$AI_CPUS")"
	say "cpus_allowed of \$\$: $(mask_of $$)"

	if [ -e /proc/set_ai_thread ]; then
		pass "/proc/set_ai_thread exists"
		say "  $(ls -l /proc/set_ai_thread 2>/dev/null)"
	else
		fail "/proc/set_ai_thread missing (CONFIG_SPACEMIT_HMP off?)"
	fi

	if [ "$(mask_of $$)" = "$REG_CPUS" ]; then
		pass "this shell runs on regular cpus"
	else
		fail "this shell mask is $(mask_of $$), expected $REG_CPUS"
		say "  run: taskset -p 0x$(cpus_to_mask "$REG_CPUS") \$\$"
	fi

	# init is the reference regular task
	if [ "$(mask_of 1)" = "$REG_CPUS" ]; then
		pass "init confined to $REG_CPUS"
	else
		fail "init mask is $(mask_of 1), expected $REG_CPUS"
	fi

	# uarch line must distinguish the two cpu classes
	if grep -q "uarch.*spacemit,a100" /proc/cpuinfo 2>/dev/null; then
		pass "/proc/cpuinfo reports spacemit,a100"
	else
		skip "/proc/cpuinfo has no spacemit,a100 uarch line"
	fi

	if [ "$(list_weight "$(cat /sys/devices/system/cpu/online)")" -eq \
	     "$(( $(list_weight "$REG_CPUS") + $(list_weight "$AI_CPUS") ))" ]; then
		pass "all cpus online, count matches layout"
	else
		fail "online cpu count does not match $REG_CPUS + $AI_CPUS"
	fi
}

# -------------------------------------------------------- test: baseline --

test_baseline() {
	hdr "baseline: default placement"

	AI_LIST=$(expand_list "$AI_CPUS" | sort -u | tr '\n' ' ')
	REG_LIST=$(expand_list "$REG_CPUS" | sort -u | tr '\n' ' ')

	# membership test against the precomputed AI cpu set
	on_ai() {
		for x in $AI_LIST; do
			[ "$x" = "$1" ] && return 0
		done
		return 1
	}
	# intersects: any cpu of this list is an AI cpu
	list_hits_ai() {
		for x in $(expand_list "$1"); do
			on_ai "$x" && return 0
		done
		return 1
	}

	bad=0
	total=0
	shown=0
	for p in /proc/[0-9]*; do
		pid=${p#/proc/}
		[ -r "$p/status" ] || continue
		m=$(mask_of "$pid")
		[ -n "$m" ] || continue
		total=$((total + 1))

		list_hits_ai "$m" || continue
		# single-cpu masks are per-cpu kthreads (cpuhp/migration/ksoftirqd/
		# kworker/N:*) and are allowed to sit on an AI cpu by design
		if [ "$(list_weight "$m")" -gt 1 ]; then
			bad=$((bad + 1))
			if [ "$shown" -lt 10 ]; then
				say "  suspicious: pid $pid $(cat "$p/comm" 2>/dev/null) mask=$m"
				shown=$((shown + 1))
				[ "$shown" -lt 10 ] || say "  ... further offenders not listed"
			fi
		fi
	done

	say "scanned $total tasks"
	if [ "$bad" -eq 0 ]; then
		pass "no multi-cpu task spans AI cpus"
	else
		fail "$bad task(s) can run on more than one AI cpu"
	fi

	# per-cpu kthreads on AI cpus are expected; count them by class
	say "tasks allowed on AI cpus (single-cpu masks expected):"
	per_cpu=0
	for p in /proc/[0-9]*; do
		pid=${p#/proc/}
		[ -r "$p/status" ] || continue
		m=$(mask_of "$pid")
		[ -n "$m" ] || continue
		list_hits_ai "$m" || continue
		if [ "$(list_weight "$m")" -eq 1 ]; then
			per_cpu=$((per_cpu + 1))
			shown=$((shown + 1))
			if [ "$shown" -lt 40 ]; then
				say "  $pid $(cat /proc/"$pid"/comm 2>/dev/null) mask=$m"
			fi
		fi
	done
	[ "$shown" -lt 40 ] || say "  ... output capped at 40 lines"
	if [ "$per_cpu" -gt 0 ]; then
		pass "$per_cpu per-cpu kthread(s) on AI cpus (expected)"
	else
		skip "no per-cpu kthread on AI cpus (are cpus $AI_CPUS online?)"
	fi

	# irq threads must stay off AI cpus (commit d715e07f7494)
	irq_ai=0
	for p in /proc/[0-9]*; do
		pid=${p#/proc/}
		c=$(cat "$p/comm" 2>/dev/null) || continue
		case $c in
		irq/*)
			m=$(mask_of "$pid")
			[ -n "$m" ] && list_hits_ai "$m" && {
				irq_ai=$((irq_ai + 1))
				[ "$irq_ai" -gt 5 ] || say "  irq on AI cpu: $pid $c mask=$m"
			}
			;;
		esac
	done
	if [ "$irq_ai" -eq 0 ]; then
		pass "no irq thread bound to AI cpus"
	else
		fail "$irq_ai irq thread(s) bound to AI cpus"
	fi
}

# ------------------------------------------------------------ test: bind --

# starts a cpu-bound task, binds it to AI cpus, verifies it executes there.
# leaves the pid in $BIND_PID.
test_bind() {
	hdr "bind: user-space AI thread"

	while :; do :; done &
	BIND_PID=$!
	say "target pid: $BIND_PID"

	m0=$(mask_of "$BIND_PID")
	c0=$(cpu_of "$BIND_PID")
	say "before: mask=$m0 cpu=$c0 state=$(state_of "$BIND_PID")"
	if [ "$m0" = "$REG_CPUS" ]; then
		pass "new task starts on $REG_CPUS"
	else
		fail "new task mask is $m0, expected $REG_CPUS"
	fi
	intersects "$c0" "$REG_CPUS" && pass "new task executes on a regular cpu" \
		|| fail "new task executing on cpu $c0"

	# the target must not have touched vector state yet, or set_ai_thread
	# warns (vector context is x100-sized and cannot be restored on a100)
	if [ ! -e /proc/set_ai_thread ]; then
		skip "/proc/set_ai_thread missing (CONFIG_SPACEMIT_HMP off?)"
		kill -9 "$BIND_PID" 2>/dev/null
		BIND_PID=
		return
	fi

	if echo "$BIND_PID" > /proc/set_ai_thread 2>/dev/null; then
		pass "write to /proc/set_ai_thread succeeded"
	else
		fail "write to /proc/set_ai_thread failed"
		kill -9 "$BIND_PID" 2>/dev/null
		BIND_PID=
		return
	fi

	m1=$(mask_of "$BIND_PID")
	say "after:  mask=$m1"
	[ "$m1" = "$AI_CPUS" ] && pass "mask is now $AI_CPUS" \
		|| fail "mask is $m1, expected $AI_CPUS"

	# the task is runnable, so __set_cpus_allowed_ptr() migrates it
	# synchronously via stop_one_cpu(); every sample must land on an AI cpu
	bad=0
	seen=""
	for i in 1 2 3 4 5; do
		c=$(cpu_of "$BIND_PID")
		seen="$seen $c"
		intersects "$c" "$AI_CPUS" || bad=$((bad + 1))
		sleep 1
	done
	say "frequencies:$(for c in $seen; do echo " $c"; done | sort | uniq -c | tr '\n' ' ')"
	[ "$bad" -eq 0 ] && pass "task executes on AI cpus ($seen )" \
		|| fail "task observed on a non-AI cpu $bad time(s)"

	# dmesg fingerprints that must not appear for a clean bind
	if command -v dmesg >/dev/null 2>&1; then
		if dmesg | grep -q "vector context has been initialized already"; then
			fail "dmesg: vector context already initialized (target used RVV)"
		else
			pass "no vector-context warning"
		fi
		if dmesg | grep -q "None ai cpu is activated"; then
			fail "dmesg: no AI cpu could be brought online"
		else
			pass "no 'None ai cpu is activated'"
		fi
	fi
}

# -------------------------------------------------------- test: restrict --

test_restrict() {
	hdr "restrict: affinity to the wrong class is rejected"

	[ -n "$BIND_PID" ] || { skip "no AI task from 'bind' step"; return; }

	REG_MASK=$(cpus_to_mask "$REG_CPUS")
	AI_MASK=$(cpus_to_mask "$AI_CPUS")
	# a proper subset of the regular cpus, e.g. 0-3 for 0-7.
	# no paste(1) here: the board's BusyBox is trimmed (no tail, no paste)
	SUB_CPUS=$(expand_list "$REG_CPUS" | sort -n | \
		awk 'NR<=4{s = s (NR > 1 ? "," : "") $1} END {print s}')
	SUB_MASK=$(cpus_to_mask "$SUB_CPUS")
	say "regular mask 0x$REG_MASK, ai mask 0x$AI_MASK, subset mask 0x$SUB_MASK"
	say "subset cpus: $SUB_CPUS (must be a proper subset of $REG_CPUS)"
	[ "$SUB_MASK" != "$REG_MASK" ] || { skip "could not build a subset"; return; }

	# C1: AI task must not go back to regular cpus
	if taskset -p 0x"$REG_MASK" "$BIND_PID" >/dev/null 2>&1; then
		fail "AI task accepted regular cpus (expected EINVAL)"
	else
		pass "AI task rejected on $REG_CPUS"
	fi
	[ "$(mask_of "$BIND_PID")" = "$AI_CPUS" ] \
		&& pass "AI task mask unchanged after rejection" \
		|| fail "AI task mask changed to $(mask_of "$BIND_PID")"

	# C2: regular task must not reach AI cpus
	if taskset -p 0x"$AI_MASK" $$ >/dev/null 2>&1; then
		fail "regular task accepted AI cpus (expected EINVAL)"
		taskset -p 0x"$REG_MASK" $$ >/dev/null 2>&1
	else
		pass "regular task rejected on $AI_CPUS"
	fi

	# C3: a proper subset of the regular cpus is allowed
	if taskset -p 0x"$SUB_MASK" $$ >/dev/null 2>&1; then
		pass "regular task accepted subset 0x$SUB_MASK"
		say "  shell mask now $(mask_of $$)"
	else
		fail "regular task rejected a legal subset 0x$SUB_MASK"
	fi
	taskset -p 0x"$REG_MASK" $$ >/dev/null 2>&1
	[ "$(mask_of $$)" = "$REG_CPUS" ] && pass "shell mask restored" \
		|| fail "shell mask is $(mask_of $$), expected $REG_CPUS"

	# D: bad pid -> ESRCH
	if echo 999999 > /proc/set_ai_thread 2>/dev/null; then
		fail "writing a nonexistent pid succeeded (expected ESRCH)"
	else
		pass "nonexistent pid rejected"
	fi
}

# --------------------------------------------------------- test: hotplug --

# offline a cpu, echo 1/0
cpu_off() { echo 0 > /sys/devices/system/cpu/cpu"$1"/online 2>/dev/null; }
cpu_on()  { echo 1 > /sys/devices/system/cpu/cpu"$1"/online 2>/dev/null; }
cpu_has_online_file() { [ -e /sys/devices/system/cpu/cpu"$1"/online ]; }

restore_all_cpus() {
	for c in $(expand_list "$REG_CPUS" | sort -n) $(expand_list "$AI_CPUS" | sort -n); do
		cpu_on "$c" >/dev/null 2>&1
	done
}

test_hotplug() {
	hdr "hotplug: cpu offline restrictions"
	say "WARNING: this offlines cpus; keep the serial console attached"

	on_before=$(cat /sys/devices/system/cpu/online)
	say "online before: $on_before"

	# ---- last regular cpu cannot go offline
	reg_list=$(expand_list "$REG_CPUS" | sort -n)
	first=$(echo "$reg_list" | sed -n '1p')
	rest=$(echo "$reg_list" | awk -v f="$first" '$1 != f')
	say "-- offline every regular cpu except cpu$first"
	for c in $rest; do
		cpu_has_online_file "$c" || { skip "cpu$c not hotpluggable"; continue; }
		cpu_off "$c" && say "   cpu$c off" || say "   cpu$c off FAILED"
	done
	if ! cpu_has_online_file "$first"; then
		skip "cpu$first has no online node; last-regular rule untestable"
	else
		if cpu_off "$first"; then
			fail "last regular cpu$first went offline"
		else
			pass "last regular cpu$first refused"
			dmesg | grep -q "Cannot offline the last Regular cpu" \
				&& pass "dmesg: last Regular cpu message" \
				|| fail "dmesg message missing"
		fi
	fi
	for c in $rest; do cpu_on "$c"; done
	say "online after : $(cat /sys/devices/system/cpu/online)"

	# ---- with an AI task alive, the last AI cpu cannot go offline
	ai_list=$(expand_list "$AI_CPUS" | sort -n)
	ai_last=$(echo "$ai_list" | sed -n '$p')
	ai_rest=$(echo "$ai_list" | awk -v l="$ai_last" '$1 != l')

	while :; do :; done &
	ai_pid=$!
	if echo "$ai_pid" > /proc/set_ai_thread 2>/dev/null; then
		pass "AI task $ai_pid created"
	else
		fail "could not create an AI task"
		kill -9 "$ai_pid" 2>/dev/null
		return
	fi
	say "-- offline every AI cpu except cpu$ai_last"
	for c in $ai_rest; do
		cpu_has_online_file "$c" || { skip "cpu$c not hotpluggable"; continue; }
		cpu_off "$c" && say "   cpu$c off" || say "   cpu$c off FAILED"
	done
	if cpu_off "$ai_last"; then
		fail "last AI cpu$ai_last went offline while an AI task exists"
	else
		pass "last AI cpu$ai_last refused with AI task alive"
		dmesg | grep -q "Cannot offline the last AI cpu" \
			&& pass "dmesg: last AI cpu message" \
			|| fail "dmesg message missing"
	fi

	# ---- after the AI task is gone, the last AI cpu must go offline
	# kill the hotplug-test AI task and any leftover user-space AI tasks
	# (e.g. a BIND_PID left by a previous 'bind' or 'all' run) before
	# checking whether the coast is clear for the last AI cpu to go offline.
	kill -9 "$ai_pid" 2>/dev/null
	wait "$ai_pid" 2>/dev/null
	# kill any remaining multi-cpu tasks that intersect AI cpus
	for _p in $(pids_multi_on_ai); do
		# skip kernel threads (kthread flag set: flags & PF_KTHREAD = 0x00200000)
		_flags=$(awk '{print $9}' /proc/"$_p"/stat 2>/dev/null)
		case $(( ${_flags:-0} & 0x200000 )) in
		0) kill -9 "$_p" 2>/dev/null; say "  killed leftover AI task $_p" ;;
		esac
	done
	sleep 1  # let the kernel reap them
	left=$(pids_multi_on_ai)
	if [ -n "$left" ]; then
		skip "tasks still allowed on AI cpus: $left"
		say "  clear them before re-running (see 'baseline')"
	else
		if cpu_off "$ai_last"; then
			pass "last AI cpu$ai_last offline with no AI task"
		else
			fail "last AI cpu$ai_last still refused with no AI task"
		fi
	fi

	restore_all_cpus
	say "online restored: $(cat /sys/devices/system/cpu/online)"
	on_after=$(cat /sys/devices/system/cpu/online)
	[ "$on_before" = "$on_after" ] && pass "cpu layout restored" \
		|| fail "cpu layout is $on_after, was $on_before"
}

# ------------------------------------------------------------------ main --

cmd=${1:-all}

# handle help and bad names first: on a kernel without HMP they still work
case $cmd in
-h|--help|help)
	sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
	exit 0
	;;
env|baseline|bind|restrict|hotplug|all) ;;
*)
	say "unknown test: $cmd"
	say "try: env | baseline | bind | restrict | hotplug | all"
	exit 2
	;;
esac

# without the proc node there is nothing to test; report it as a skip
# (kselftest convention: 4) rather than as a failure
if [ ! -e /proc/set_ai_thread ]; then
	say "SKIP: /proc/set_ai_thread missing (CONFIG_SPACEMIT_HMP off?)"
	exit 4
fi

case $cmd in
env)      test_env ;;
baseline) test_baseline ;;
bind)     test_bind; say ""; say "target pid left running: ${BIND_PID:-none}" ;;
restrict) test_restrict ;;
hotplug)  test_hotplug ;;
all)
	test_env
	test_baseline
	test_bind
	test_restrict
	[ -n "$BIND_PID" ] && { kill -9 "$BIND_PID" 2>/dev/null; wait "$BIND_PID" 2>/dev/null; say "target pid $BIND_PID cleaned up"; BIND_PID=; }
	;;
esac

hdr "summary"
say "  passed: $PASS"
say "  failed: $FAIL"
say "  skipped: $SKIP"
[ "$FAIL" -eq 0 ] || exit 1
exit 0
