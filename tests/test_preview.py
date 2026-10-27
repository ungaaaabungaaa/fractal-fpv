from http.client import HTTPConnection
from pathlib import Path
import tempfile
import threading
import unittest
from scripts.preview import make_server


class PreviewTests(unittest.TestCase):
    def test_public_only_loopback_server_has_no_write_or_directory_interface(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "index.html").write_text("<p>Fractal preview</p>")
            (root / "private.key").write_text("private")
            server = make_server(root, 0)
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()

            def request(path, method="GET", host=None):
                client = HTTPConnection("127.0.0.1", server.server_port, timeout=3)
                try:
                    client.request(
                        method,
                        path,
                        headers={"Host": host or f"127.0.0.1:{server.server_port}"},
                    )
                    response = client.getresponse()
                    return response.status, dict(response.getheaders()), response.read()
                finally:
                    client.close()

            try:
                status, headers, body = request("/")
                self.assertEqual(status, 200)
                self.assertEqual(body, b"<p>Fractal preview</p>")
                self.assertIn("script-src 'self'", headers["Content-Security-Policy"])
                self.assertEqual(request("/", "HEAD")[2], b"")
                self.assertEqual(request("/", "POST")[0], 405)
                self.assertEqual(request("/", host="attacker.example")[0], 403)
                for path in [
                    "/private.key",
                    "/assets/",
                    "/../Source/FractalFPV/FractalCore/FractalCore.h",
                ]:
                    self.assertEqual(request(path)[0], 404)
                (root / "app.js").write_bytes(b"x" * 1048577)
                self.assertEqual(request("/app.js")[0], 404)
            finally:
                server.shutdown()
                server.server_close()
                thread.join(timeout=2)


if __name__ == "__main__":
    unittest.main()
