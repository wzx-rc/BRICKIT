"""BrickIt - publishes the full LDraw library (the generated folder %USERPROFILE%\\BrickIt\\full) into the data branch
`bibliotheque` of the BRICKIT repository: one source for the PC and the iPad build (GitHub Actions bundles it in the .ipa,
tools/brickit/library_fetch.py gets it back anywhere). The branch has no history (each publish replaces it) so the
repository does not grow with every new version of the library. Files over 95 MB (thumbs.pack) are cut in parts
(.part000, .part001...), GitHub refusing files over 100 MB; library_fetch.py joins them.

    py tools\\brickit\\library_publish.py [full folder] [repository folder]
    (defaults: %USERPROFILE%\\BrickIt\\full and the repository this script is in)
"""
import os, shutil, subprocess, sys, tempfile, time

CHUNK = 95 * 1024 * 1024
SKIP_DIRS = True                      # thumbs_v3_parts\ (one image per part, resumable render) stays on the PC
SKIP = ("backup",)                    # thumbs_v1_backup.pack


def run(args, cwd):
    print(">", " ".join(args))
    subprocess.check_call(args, cwd=cwd)


def main():
    full = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.expanduser("~"), "BrickIt", "full")
    repo = sys.argv[2] if len(sys.argv) > 2 else os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if not os.path.isfile(os.path.join(full, "index.json")):
        sys.exit("no library in " + full + " (index.json missing)")
    work = tempfile.mkdtemp(prefix="brickit_lib_")
    try:
        run(["git", "worktree", "add", "--detach", work], repo)
        run(["git", "checkout", "--orphan", "bibliotheque-new"], work)
        run(["git", "rm", "-rfq", "--cached", "."], work)
        for name in os.listdir(work):
            if name == ".git":
                continue
            p = os.path.join(work, name)
            shutil.rmtree(p) if os.path.isdir(p) else os.remove(p)
        dst = os.path.join(work, "full")
        os.makedirs(dst)
        total, count = 0, 0
        for name in sorted(os.listdir(full)):
            src = os.path.join(full, name)
            if os.path.isdir(src) or any(k in name for k in SKIP):
                continue
            size = os.path.getsize(src)
            total += size
            if size <= CHUNK:
                shutil.copy2(src, os.path.join(dst, name))
                count += 1
                continue
            with open(src, "rb") as f:
                k = 0
                while True:
                    data = f.read(CHUNK)
                    if not data:
                        break
                    with open(os.path.join(dst, "%s.part%03d" % (name, k)), "wb") as o:
                        o.write(data)
                    k += 1
                    count += 1
        with open(os.path.join(work, "README.md"), "w", encoding="utf-8") as f:
            f.write("# Bibliothèque complète LDraw de BrickIt (données)\n\n"
                    "Branche de données, sans historique : `tools/brickit/library_publish.py` (branche main) la remplace "
                    "à chaque publication. Voir docs/brickit/BIBLIOTHEQUE_COMPLETE.md.\n\n"
                    "Publiée le %s : %d fichiers, %.0f Mo.\n" % (time.strftime("%Y-%m-%d %H:%M"), count, total / 1e6))
        run(["git", "add", "-A"], work)
        run(["git", "-c", "core.autocrlf=false", "commit", "-qm", "Bibliothèque complète LDraw (%.0f Mo)" % (total / 1e6)], work)
        run(["git", "push", "-f", "origin", "HEAD:refs/heads/bibliotheque"], work)
        print("published: %d files, %.0f MB" % (count, total / 1e6))
    finally:
        subprocess.call(["git", "worktree", "remove", "--force", work], cwd=repo)
        subprocess.call(["git", "branch", "-D", "bibliotheque-new"], cwd=repo)


if __name__ == "__main__":
    main()
