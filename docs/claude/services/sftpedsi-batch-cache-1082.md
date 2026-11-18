---
type: rule
title: "SftpEDSI::Batch: one script per folder click, cache dropped on any change (#1082)"
area: services
paths: ["src/dsm/edsiimpl/SftpEDSI.C", "include/dsm/SftpEDSI.H", "src/dsm/edsiimpl/Resource.C", "src/apps/organizer/ProjectContextPanel.C", "tests/launch/ssh_folder_rtt_test.py"]
issues: [1082]
---
- Every SftpEDSI read is one remote script (a round trip), so a folder click
  cost 130+ scripts (per child metadata, per calculation a depth-1 and a
  depth-1000 mirror, Props probes for the energy column).
- `SftpEDSI::Batch` (RAII, per thread, nests) is held in `Resource::getChildren`
  and `ProjectContextPanel::initialize`. Inside it the first depth-1 `mirror`
  of a path sends one fill script: the folder at depth 1, each child folder
  at depth 1, and each child whose `.ecce-meta` says `virtual_document` in
  full, plus base64 of its `Props/*` files up to 64 kB. The raw output is kept
  in memory only; later `mirror`, `exists`, `getDataSet` are rebuilt from it
  (a path under a listed folder and not in it is "absent" with no round trip).
- Rule: any operation that may change the server (`SftpOp::run` default,
  `lock`, `upload`, `release`) bumps `gGen`, which invalidates the cache, and
  an `SftpOp` that has locked or written never reads from it. A new read-only
  script must pass `mutates=false` to `run`; a new mutation needs nothing.
- The cache dies with the outermost Batch. Never persist it: the folder is
  shared with other clients and jobs.
- `ECCE_SFTP_NOBATCH=1` disables it; `server rtt` (organizer test command)
  gives the script count. `ssh_folder_rtt_test.sh` compares both.
