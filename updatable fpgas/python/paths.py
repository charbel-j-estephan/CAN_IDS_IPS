"""Where things are. data/ and models/ are not in git, so in a git worktree they
are only in the main checkout: look in this checkout first, then that one."""
import os
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
UPDATABLE = os.path.dirname(HERE)              # the "updatable fpgas" folder
REPO = os.path.dirname(UPDATABLE)
BUILD = os.path.join(UPDATABLE, "build")
IVERILOG = r"C:\iverilog\bin\iverilog.exe"
VVP = r"C:\iverilog\bin\vvp.exe"
if os.name != "nt":                            # Linux or macOS: Icarus from the PATH
    IVERILOG, VVP = "iverilog", "vvp"


def _main_checkout():
    try:
        common = subprocess.check_output(["git", "rev-parse", "--git-common-dir"], cwd=REPO,
                                         text=True, stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError):
        return REPO
    if not os.path.isabs(common):
        common = os.path.join(REPO, common)
    return os.path.dirname(os.path.abspath(common))


def find_shared(*parts):
    """A path under the repo root, in this checkout or the main one."""
    for root in (REPO, _main_checkout()):
        p = os.path.join(root, *parts)
        if os.path.exists(p):
            return p
    raise SystemExit("Not found: %s (looked in %s and %s)"
                     % (os.path.join(*parts), REPO, _main_checkout()))
