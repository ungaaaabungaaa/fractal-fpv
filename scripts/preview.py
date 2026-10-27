"""Fixed public assets only. No compiler, Unreal launch, or write endpoint."""

import argparse
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[1]
ASSETS = {
    "index.html": "text/html; charset=utf-8",
    "styles.css": "text/css; charset=utf-8",
    "app.js": "text/javascript; charset=utf-8",
    "view-model.js": "text/javascript; charset=utf-8",
    **{f"assets/mesh-{depth}.json": "application/json" for depth in range(3)},
    "assets/camera-track.json": "application/json",
}


def make_server(public=None, port=8796):
    public = Path(public or ROOT / "web").resolve()

    class Handler(BaseHTTPRequestHandler):
        server_version = "FractalPreview"
        sys_version = ""

        def setup(self):
            super().setup()
            self.connection.settimeout(2)

        def log_message(self, *_args):
            pass

        def reply(self, code, body, kind="text/plain; charset=utf-8"):
            if isinstance(body, str):
                body = body.encode("utf8")
            self.send_response(code)
            for name, value in {
                "Content-Security-Policy": "default-src 'self'; script-src 'self'; style-src 'self'; connect-src 'self'; img-src 'self'; object-src 'none'; frame-ancestors 'none'; base-uri 'none'; form-action 'none'",
                "X-Content-Type-Options": "nosniff",
                "Referrer-Policy": "no-referrer",
                "Cache-Control": "no-store",
                "Content-Type": kind,
                "Content-Length": str(len(body)),
                "Connection": "close",
            }.items():
                self.send_header(name, value)
            self.end_headers()
            if self.command != "HEAD":
                self.wfile.write(body)

        def send_error(self, code, message=None, explain=None):
            self.reply(405 if code == 501 else code, "Unsupported request.")

        def do_HEAD(self):
            self.do_GET()

        def do_GET(self):
            if len(self.headers.get_all("Host", [])) != 1 or self.headers.get(
                "Host"
            ) not in {
                f"127.0.0.1:{self.server.server_port}",
                f"localhost:{self.server.server_port}",
            }:
                self.reply(403, "A matching loopback Host is required.")
                return
            try:
                if len(self.path) > 2048:
                    raise ValueError("Request too large")
                url = urlsplit(self.path)
                name = "index.html" if url.path == "/" else url.path.lstrip("/")
                if (
                    url.scheme
                    or url.netloc
                    or url.query
                    or url.fragment
                    or name not in ASSETS
                ):
                    self.reply(404, "Public asset not found.")
                    return
                path = public / name
                if (
                    path.is_symlink()
                    or not path.is_file()
                    or not path.resolve().is_relative_to(public)
                ):
                    raise OSError("Not a public file")
                with path.open("rb") as stream:
                    data = stream.read(1048577)
                if len(data) > 1048576:
                    raise OSError("Oversized asset")
                self.reply(200, data, ASSETS[name])
            except (OSError, ValueError):
                self.reply(404, "Public asset unavailable.")

    class Server(HTTPServer):
        allow_reuse_address = True

    return Server(("127.0.0.1", port), Handler)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Read-only browser preview; Unreal is not invoked."
    )
    parser.add_argument("--port", type=int, default=8796)
    args = parser.parse_args()
    if not 1 <= args.port <= 65535:
        parser.error("Port must be from 1 to 65535.")
    server = make_server(port=args.port)
    print(
        f"http://127.0.0.1:{args.port}/ — browser preview, Unreal validation pending",
        flush=True,
    )
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
