// ////////////////////////////////////////////////////////////////////////////////
// noobWarrior
// Plugin: HTTP Server Base
// File: forum-admin.js
// Description:
// Started by: Hattozo
// Started on: 6/25/2026
// ////////////////////////////////////////////////////////////////////////////////
(function () {
    "use strict";

    // Briefly show a message in the bottom-right corner.
    function toast(message) {
        var host = document.getElementById("fa-toasts");
        if (!host) return;
        var el = document.createElement("div");
        el.className = "fa-toast";
        el.textContent = message;
        host.appendChild(el);
        setTimeout(function () { el.remove(); }, 2500);
    }

    // Replace the current forums list with the one in a freshly rendered page.
    function swapForumAdmin(html) {
        var page = new DOMParser().parseFromString(html, "text/html");
        var fresh = page.getElementById("forum-admin");
        var current = document.getElementById("forum-admin");
        if (fresh && current) current.replaceWith(fresh);
    }

    document.addEventListener("submit", function (e) {
        var form = e.target;
        if (!form.classList.contains("fa-ajax")) return;
        e.preventDefault();

        if (form.dataset.confirm && !confirm(form.dataset.confirm)) return;

        var button = form.querySelector("button[type=submit]");
        if (button) button.disabled = true;

        fetch(form.action, {
            method: "POST",
            body: new URLSearchParams(new FormData(form))
        }).then(function (response) {
            // The endpoint redirects (200 after following) on success, or returns a 4xx
            // with an error message we can show.
            return response.text().then(function (body) {
                if (response.ok) {
                    swapForumAdmin(body);
                    toast(form.dataset.okMsg || "Saved");
                } else {
                    toast(body.trim() || "Something went wrong.");
                    if (button) button.disabled = false;
                }
            });
        }).catch(function () {
            toast("Network error.");
            if (button) button.disabled = false;
        });
    });
})();
