"""BrickIt - gets the full LDraw library from the data branch `bibliotheque` (see library_publish.py) into a folder,
the cut files joined again. Used by the iPad build (GitHub Actions: assets/brickit/full, bundled in the .ipa); on the PC
it can fill %USERPROFILE%\\BrickIt\\full on a new machine.

    python tools/brickit/library_fetch.py <destination folder> [repository folder]
"""
import io, os, re, subprocess, sys, tarfile


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    dest = sys.argv[1]
    repo = sys.argv[2] if len(sys.argv) > 2 else os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    os.makedirs(dest, exist_ok=True)
    fetch_branch("bibliotheque", dest, repo)
    # the HD thumbnails (thumbs_hq.pack, 512 px, --build-hq-thumbs on the PC) when published: branch vignettes-hd
    if subprocess.run(["git", "ls-remote", "--exit-code", "--heads", "origin", "vignettes-hd"], cwd=repo, stdout=subprocess.DEVNULL).returncode == 0:
        fetch_branch("vignettes-hd", dest, repo)
    n = len(os.listdir(dest))
    if not os.path.isfile(os.path.join(dest, "index.json")):
        sys.exit("library incomplete in " + dest)
    print("library: %d files in %s" % (n, dest))


def fetch_branch(branch, dest, repo):
    subprocess.check_call(["git", "fetch", "--depth", "1", "origin", branch], cwd=repo)
    tar = subprocess.run(["git", "archive", "--format=tar", "FETCH_HEAD", "full"], cwd=repo, check=True, stdout=subprocess.PIPE).stdout
    parts = {}
    with tarfile.open(fileobj=io.BytesIO(tar)) as t:
        for m in t.getmembers():
            if not m.isfile():
                continue
            name = os.path.basename(m.name)
            data = t.extractfile(m).read()
            cut = re.match(r"(.+)\.part(\d{3})$", name)
            if cut:
                parts.setdefault(cut.group(1), {})[int(cut.group(2))] = data
                continue
            with open(os.path.join(dest, name), "wb") as f:
                f.write(data)
    for name, chunks in parts.items():
        with open(os.path.join(dest, name), "wb") as f:
            for k in sorted(chunks):
                f.write(chunks[k])
    print("%s: fetched" % branch)


if __name__ == "__main__":
    main()
