# AGENTS.md — raimd

Rai Market Data: message codecs and dictionaries. One `MDMsg` interface
over every wire format the fleet speaks — RVMSG, TIBMSG, TIB_SASS (SASS2/3
QFORM), RWF (field list / map / series, the OMM message set), Marketfeed
(`MktfdMsg`), JSON, plus writers for each — and the dictionary loader for
TIB cfiles, RDM `RDMFieldDictionary`/`enumtype.def` (`app_a`) and
`flistmapping`. Also the redis-style data types used by raids (list, set,
zset, stream, hll, geo) because they share the `MDMsg` memory model.
Namespace `rai::md`. Depends on `libdecnumber` only.

## Workspace layout (injinj fleet convention)

- This repo is one of the `injinj` siblings. Every dependency is a sibling
  checkout (`../libdecnumber`); headers come from
  `-I../<dep>/include` and libraries from
  `../<dep>/FC43_x86_64/lib64/lib<dep>.a`. There is no install step between
  siblings — build the dependency, then build this repo.
- Dependencies are declared once, in `build_depends.mak`
  (`<dep>_dep := MAJOR.MINOR`, the rpm BuildRequires minimum). `./deps.sh`
  clones missing siblings from it, `./deps.sh -b` also builds them in
  dependency order, `./deps.sh -s` prints the commit of this repo and every
  sibling (the only record of what a release was built from).
- Whole-fleet build order lives in `~/injinj/GNUmakefile` (`make build`):
  openpgm → raikv → libdecnumber → raimd → linecook → lzf → rdbparser → h3 →
  HdrHistogram_c → sassrv → raids → natsmd → omm → raims → rvcache →
  raimdapi → ossl.

## Build

- `make` → `FC43_x86_64/bin`, `FC43_x86_64/lib64` (static `.a` + versioned
  `.so`). The build dir is `$(short_dist)$(lsb_dist_ver)_$(uname_m)$(port_extra)`:
  Fedora 43 → `FC43_x86_64`, Rocky 9.5 → `ROCKY9.5_x86_64`.
- `make port_extra=-g` → debug build in `FC43_x86_64-g`.
  `make port_extra=-mingw` → Windows cross build in `FC43_x86_64-mingw`
  (mingw64 toolchain; the mingw build links sibling *master* checkouts).
- `NO_STL=1` builds with `-fno-rtti -fno-exceptions` and links with the C
  compiler; the library code is written to survive that (error returns, no
  exceptions, no RTTI, no STL containers in public headers). Keep it that way.
- `make -j8` is safe; the makefile dependencies are complete (verified clean
  and incremental). One trap: the **first `-j` run right after a compile
  error** can fail test executables with bogus undefined references
  (symbols from this repo's own lib) because they link against a library
  left half-updated by the failed run. Run plain `make` once, then `-j`
  works again. It is not a missing dependency — don't go looking for one.
- Warnings are clean. A new warning is a defect.
- If a sibling's public headers changed, rebuild the sibling first. A link
  warning like `lib<dep>.so.1.N, needed by ../<x>/.../lib<x>.so, not found`
  means sibling `<x>` was built against an older `<dep>` — rebuild `<x>`;
  the warning becomes a hard failure at run time.
- Ad hoc test program without touching the GNUmakefile:
  `g++ -std=c++11 -O1 -g -Wall -Wextra -Iinclude -I../<dep>/include -o /tmp/x
  test/x.cpp -LFC43_x86_64/lib64 -lraimd -L../<dep>/FC43_x86_64/lib64 -l<dep> …`
  then run with `LD_LIBRARY_PATH` pointing at each `lib64` (the link picks
  the `.so`). For repos with many siblings, capture the makefile's own
  lines once: `touch src/<any>.cpp; make -n | grep -- '-c src/<any>.cpp'`
  and `make -n | grep -- '-o FC43_x86_64/bin/' | head -1`, then reuse the
  compile line (minus `-c/-o`) and the link line's `-Wl,-rpath` + lib tail.
- Dictionary files for any test: `~/injinj/rmds-config` (TIB cfiles
  `tss_fields.cf`/`tss_records.cf` with their `COMMON_/SYS_/REUT_/RAI_/TEST_`
  includes, `RDMFieldDictionary`, `enumtype.def`, `flistmapping`; 5275
  classes, 268 forms). `load_dict_files(path)` loads all of them.
- Replay captures `rsf4_<codec>.dat` (rv, rvmsg, tibmsg, tibsass..tibsass7,
  mf, rwf) live in this repo root and are the input for sassrv's
  `replayrv` and the codec tests.

## Versioning and release

- Version is `major_num.minor_num.patch_num-build_num` in `.copr/Makefile`
  (included by the GNUmakefile). `make spec_version` prints the
  `raimd_dep := MAJOR.MINOR` line that dependents pin in their
  `build_depends.mak`.
- `rpm/raimd.spec` carries placeholders (`Version: 999.999`, release
  `99999`, `BuildRequires: <dep> _<dep>_dep`); `make srpm` substitutes them
  from `build_depends.mak` and tars `git ls-tree HEAD` into
  `rpmbuild/SOURCES`. Keep the `_ensure_git` line in `.copr/Makefile`:
  COPR's `make_srpm` chroot has no git and every `$(shell git …)` would
  silently expand empty.
- COPR: `injinj/dev` builds the `dev` branch of github.com/injinj/raimd
  on push (webhook); `injinj/rel` builds github.com/raitechnology/raimd.
  Remotes: `origin` = raitechnology (release), `ii` = injinj (dev).
  Chroots: epel-7/8/9/10, fedora-43/44/45/rawhide, x86_64.
- ABI convention across the fleet: a dependent's rpm `Requires` is
  `MAJOR.MINOR` and its `BuildRequires` is the full version; the MINOR is
  bumped together with any change that alters the C++ ABI of a public
  header, so a rebuilt dependent never silently loads an old `.so`.
  Bump `minor_num` here when public headers change, then re-pin and
  rebuild the dependents.
- Link only against installable sibling libraries (`$(lnk_lib)`), never a
  sibling's object files; if a binary needs a sibling helper, move the
  source into the sibling's library and release the sibling first.
- Signed rpms are published to https://www.raitechnology.com/repo/
  (COPR rel → download → sign → createrepo → rsync; tooling in
  `~/injinj/rel`).

## Code conventions

- C++11, two-space indent, `/* … */` block comments, aligned member
  comments; snake_case functions/variables, CamelCase types.
- Namespaces: `rai::<repo-namespace>` (see below). System headers first,
  then project headers.
- Errors are return codes / status enums; no exceptions, no RTTI in library
  code. Raw pointers and placement-new over allocators are the norm; look
  at how the neighbouring code allocates before adding `new`/`delete`.
- Keep the generated `CMakeLists.txt` out of your edits — it is derived from
  the GNUmakefile (and slated for removal).

## Layout

- `include/raimd/md_msg.h` — `MDMsg`, `MDMsgMem`, `MDFieldIter`,
  `MDReference`, codec registration (`md_match_ftype[]`,
  `first_match/next_match`), `MDMsg::unpack()`.
- Per-codec: `rv_msg.h`, `tib_msg.h`, `tib_sass_msg.h`, `rwf_msg.h` +
  `rwf_writer.h` + `omm_msg.h`/`omm_flags.h`, `mf_msg.h` (Marketfeed),
  `json_msg.h`, `sass.h` (SASS header/record constants).
- Dictionaries: `md_dict.h` (`MDDict`, `MDLookup`, `MDDictBuild`,
  `MDFormClass`), `cfile.h` (TIB cfile parser + `pack_sass`/`pack_sass3`),
  `app_a.h` (RDM), `enum_def.h`, `flistmap.h`, `dict_load.h`.
- `src/` mirrors the headers; `test/` has one program per codec/type
  (`test_msg`, `test_rwf`, `test_mf_writer`, `test_sass3`, `md_test_dict`,
  `test_mddec`, …) and the `read_msg`/`write_msg`/`pretty_js` tools.
- `golang/` — Go bindings (cgo).

## Contracts and gotchas (verified, keep current)

### MDMsg / codecs
- A codec is identified two ways: the one-byte `ftype` (what the hash
  entry type byte stores) and the 32-bit id (`MDMatch::hint[0]`, what
  `EvPublish.msg_enc` and sassrv's `make_rv_msg` switch on). Convert byte
  → id by iterating `first_match/next_match` and matching `ma->ftype`.
  `MDMsg::unpack(..., h)` accepts either (`h < 256` → `md_match_ftype[]`).
- Generic writers: `msg->create_writer(w, mem, dict, buf, len)` returns the
  message's own codec writer (`MDMsgWriterBase`: `append_iter`,
  `update_hdr`, `convert_msg`, `append_sass_hdr`). There is **no** generic
  `append_ref/append_uint` — those are per-codec; fall back to
  `RvMsgWriter` when a codec has no writer.
- RWF field lists carry no MSG_TYPE field; a dictionary is required to
  iterate them. Delivery kind comes from `RwfMsgPeek::get_msg_class()`
  (REFRESH=2 / STATUS=3 / UPDATE=4, `omm_flags.h`).
- Marketfeed encode from RWF goes through the field iterator
  (`MktfdMsgWriter`), no intermediate model; ossl relies on this.

### Dictionaries (`MDDict`)
- `load_dict_files(path)` returns a list: `cfile` → `app_a` → `flist`
  (`dict_type[0]` is `'c'`/`'a'`/`'f'`). Walk a dict by fid:
  `for fid in min_fid..max_fid { MDLookup by(fid); dict->lookup(by); }`,
  by name with `dict->get(MDLookup(name, len))`.
- **`MDLookup::fname_len` includes the terminating NUL** (`strlen + 1`).
  Copy `fname_len` bytes and you have a terminated string; add one and
  every name hash misses.
- Form classes are `MD_MESSAGE` entries with `map_num != 0`;
  `dict->get_form_class(fid)` gives `entries[]{fid, foffset}` and
  `form_size`. `foffset` is per form (a fid sits in many forms) — never
  store it on the fid entry.
- `MDFormClass` keeps only fids. The cfile `FIELD_CLASS_NAME` alias
  (`"symbol" { FIELD_CLASS_NAME "Output Ticker Symbol"; }` in
  `tss_records.cf`) is parsed by `CFile` but not stored, so anything built
  from `MDDict` sees the class name, not the form-local name.
  <!-- TODO: decide whether MDFormBuild/MDFormClass should carry per-field names -->
- The TSS data type is not stored; `CFile::md_to_tss_type()` re-derives it
  from `MDType` + `fsize`. Ambiguous cases in real dictionaries: 8-byte
  `TSS_DOUBLE` ↔ `TSS_DOUBLE_INT`, 8-byte `TSS_INTEGER` ↔ `TSS_LONG`,
  partial `TSS_OPAQUE` → `MD_PARTIAL` → `TSS_STRING`. `MD_OPAQUE` →
  `TSS_OPAQUE` and `MD_BOOLEAN` → `TSS_BOOLEAN` were added 2026-10-15;
  older libraries sent OPAQUE as STRING and dropped BOOLEAN fids from
  `pack_sass`.
- Wire formats: `CFile::pack_sass` is the RaiCore/raimdapi packed
  dictionary (`MAX_FID` u16, `NAME_BYTES`, `FIDS{fname: u16 fid, u32 hint =
  partial<<24 | tss<<16 | fsize}`). `pack_sass3/unpack_sass3` add the
  `FORMS` submessage and the legacy `MARKETFEED` submessage (capr_svc
  format): integers inside its opaque `a/e/l/m/w` records are
  **big-endian**, its Marketfeed type numbers differ from raimd's `MF_*`,
  and `fenum` is raimd's `map_num`. Round trip: `test/test_sass3.cpp` —
  dumps may differ only in form `map_num` order; repack must be
  byte-identical.
- Unaligned reads everywhere: messages arrive in network buffers with no
  alignment guarantee; use the existing helpers (e.g. `get_uint<uint64_t>`, memcpy-based)
  rather than casting a `uint8_t *` to a wider integer pointer.

## Tests

- `make` builds every `test/*.cpp` into `FC43_x86_64/bin`. The codec
  tests read the `rsf4_*.dat` captures; dictionary tests take the
  `rmds-config` path as an argument. <!-- TODO: canonical invocation per test -->
- Run with `LD_LIBRARY_PATH=FC43_x86_64/lib64:../libdecnumber/FC43_x86_64/lib64`
  when running from outside the make environment.

## Open items / history

- 2026-10-15: `pack_sass3/unpack_sass3` moved here from raimdapi
  (`src/cfile.cpp`); raimdapi is being changed to build its dictionary
  from `MDDict` (`RaiMsg_config::fromMDDict`). `md_to_tss_type` became a
  public static; `tss_data_type_to_md_type` / `tss_type_default_size` are
  still file-static.
<!-- TODO: fill from git log -->
