function SetupSubmissionUploadSystem() {
    const CHUNK_SIZE = 1024 * 1024; // 1 MiB chunks
    const btn = document.getElementById("ws-upload-btn");
    const statusEl = document.getElementById("ws-status");
    const progress = document.getElementById("ws-progress");

    async function postJson(url, obj) {
        const res = await fetch(url, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(obj),
        });
        if (!res.ok) throw new Error((await res.text()) || res.statusText);
        return res.json();
    }

    btn.addEventListener("click", async function () {
        const name = document.getElementById("ws-name").value.trim();
        const kind = document.getElementById("ws-type").value;
        const description = document.getElementById("ws-desc").value;
        const file = document.getElementById("ws-file").files[0];

        if (!name) { statusEl.textContent = "Please enter a name."; return; }
        if (!file) { statusEl.textContent = "Please choose a file."; return; }

        btn.disabled = true;
        progress.style.display = "block";
        progress.value = 0;
        statusEl.textContent = "Starting upload...";

        try {
            const start = await postJson("/v1/workshop/start-upload", {
                Name: name,
                Description: description,
                Type: kind,
                Filename: file.name,
                Size: file.size,
            });
            const uploadToken = start.UploadToken;

            let offset = 0;
            while (offset < file.size) {
                const slice = file.slice(offset, offset + CHUNK_SIZE);
                const res = await fetch(
                    "/v1/workshop/stream-upload?token=" + encodeURIComponent(uploadToken),
                    { method: "POST", headers: { "Content-Type": "application/octet-stream" }, body: slice }
                );
                if (!res.ok) throw new Error((await res.text()) || res.statusText);
                offset += slice.size;
                progress.value = file.size ? Math.floor((offset / file.size) * 100) : 100;
                statusEl.textContent = "Uploading... " + progress.value + "%";
            }

            const done = await postJson("/v1/workshop/end-upload", { UploadToken: uploadToken });
            progress.value = 100;
            if (done.SubmissionId) {
                statusEl.innerHTML = "Upload complete! Submission #" + done.SubmissionId +
                    ' created. <a href="/v1/workshop/download?id=' + done.SubmissionId + '">Download</a>';
            } else {
                statusEl.textContent = "Upload complete!";
            }
        } catch (err) {
            statusEl.textContent = "Upload failed: " + err.message;
        } finally {
            btn.disabled = false;
        }
    });
}