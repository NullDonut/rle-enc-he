#!/bin/bash
# Round-trip, boundary, fuzz and corruption tests for the RLE tool.
# Usage: ./test.sh [path-to-rle-binary]    (default: ./rle)
# CLI under test:  rle <file>  -> enc.rle     rle -d <file.rle>  -> out.bin
# Runs inside a temp directory so enc.rle / out.bin never touch the repo.

RLE=$(realpath "${1:-./rle}")
[ -x "$RLE" ] || { echo "binary not found: $RLE (run make first)"; exit 2; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
cd "$T" || exit 2
pass=0; fail=0

ok()  { pass=$((pass+1)); }
bad() { fail=$((fail+1)); echo "FAIL $1"; }

# repeat BYTE_HEX N  -> N copies of that byte
rep() { python3 -c "import sys; sys.stdout.buffer.write(bytes([0x$1])*$2)"; }
# alternating AB pattern of length N (no runs)
alt() { python3 -c "import sys; sys.stdout.buffer.write((b'AB'*$1)[:$1])"; }

roundtrip() {   # name, file
    if "$RLE" "$2" 2>/dev/null && "$RLE" -d enc.rle 2>/dev/null && cmp -s "$2" out.bin; then ok
    else bad "roundtrip $1"; fi
    rm -f enc.rle out.bin
}

# ---- fixed cases ----
: > empty;                         roundtrip empty empty
printf x > one;                    roundtrip one-byte one
printf 'aaaabbbccd' > simple;      roundtrip simple simple
printf 'aaa\nbbb' > nonl;          roundtrip no-trailing-newline nonl
printf '1112223333' > digits;      roundtrip digits digits
printf 'abcdefghij' > norun;       roundtrip no-runs norun
printf 'a\r\n\r\nb' > crlf;        roundtrip crlf crlf
printf 'a\0\0\0b\0' > nuls;        roundtrip nul-bytes nuls
head -c 20000 /dev/urandom > rnd;  roundtrip random rnd
for n in 1 2 3 127 128 129 130 255 256 257 4095 4096 4097 5000 100000; do
    rep 00 $n > "run$n"; roundtrip "run-$n" "run$n"
done
for n in 1 2 127 128 129 256 4097; do
    alt $n > "alt$n"; roundtrip "alternating-$n" "alt$n"
done
printf 'abcaaaaxyzbbbbb' > mixed;  roundtrip mixed mixed
printf 'aab' > t1;                 roundtrip run2-then-literal t1
{ rep 61 200; printf 'xyz'; rep 62 3; } > t2; roundtrip run-literal-run t2

# ---- format checks (v1 spec) ----
printf 'aaaabbbccd' > spec; "$RLE" spec
exp='524c4531 0a000000 0400000061 0300000062 0200000063 0100000064'
got=$(od -An -v -tx1 enc.rle | tr -d ' \n')
[ "$got" = "$(echo $exp | tr -d ' ')" ] && ok || bad "format bytes: $got"
: > empty2; "$RLE" empty2
[ "$(od -An -v -tx1 enc.rle | tr -d ' \n')" = "524c453100000000" ] && ok || bad "empty file header"
rm -f enc.rle

# ---- corruption: must be rejected, exit non-zero, leave no out.bin ----
"$RLE" spec; cp enc.rle good.rle; rm -f enc.rle
head -c 6  good.rle > c_trunc_header
head -c 12 good.rle > c_trunc_record
head -c -5 good.rle > c_missing_record
printf 'XXXX\0\0\0\0'          > c_bad_magic
printf 'RLE1\x05\0\0\0\0\0\0\0a' > c_zero_run
printf 'RLE1\x02\0\0\0\x09\0\0\0a' > c_too_long
printf 'RLE1\x0a\0\0\0\x04\0\0\0a' > c_length_mismatch
: > c_empty_file
for f in c_*; do
    rm -f out.bin
    if "$RLE" -d "$f" 2>/dev/null; then bad "accepted corrupt: $f"
    elif [ -e out.bin ]; then bad "left out.bin after: $f"
    else ok; fi
done

# ---- CLI errors ----
"$RLE" nonexistent 2>/dev/null && bad "missing input accepted" || ok
"$RLE" 2>/dev/null && bad "no args accepted" || ok
"$RLE" -d nonexistent 2>/dev/null && bad "missing -d input accepted" || ok
# output path unusable: must fail cleanly (exit 1), not crash
mkdir -p blocked/enc.rle; printf aaa > blocked/in
( cd blocked && "$RLE" in 2>err; rc=$?; [ $rc -eq 1 ] && ! grep -q Sanitizer err ) && ok || bad "unwritable enc.rle: crash or wrong exit"

# ---- fuzz: 300 random run-heavy files ----
for i in $(seq 1 300); do
    python3 - "$i" > fz <<'PY'
import random, sys
random.seed(int(sys.argv[1]))
out = bytearray()
for _ in range(random.randint(0, 40)):
    out += bytes([random.randrange(256)]) * random.choice([1,1,2,3,5,127,128,129,300,4097])
sys.stdout.buffer.write(out)
PY
    if "$RLE" fz 2>/dev/null && "$RLE" -d enc.rle 2>/dev/null && cmp -s fz out.bin; then ok
    else bad "fuzz seed $i"; cp fz "$OLDPWD/fuzz_fail_$i.bin" 2>/dev/null; fi
    rm -f enc.rle out.bin
done

echo "passed: $pass  failed: $fail"
[ $fail -eq 0 ]
