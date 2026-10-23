"""Editor automation bridge (development only).

When Saved/ClaudeBridge.enabled exists, the editor listens on 127.0.0.1:8765 (a -game process on 8766) and runs posted Python
on the game thread (used by the agentic build workflow). Without that file this script does nothing.
"""
import os

import unreal

_FLAG = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "ClaudeBridge.enabled")

if os.path.exists(_FLAG):
    import contextlib
    import io
    import json
    import queue
    import threading
    import traceback
    from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

    _jobs = queue.Queue()
    _scope = {"unreal": unreal, "__name__": "__bridge__"}

    class _Handler(BaseHTTPRequestHandler):
        def do_POST(self):
            length = int(self.headers.get("Content-Length", 0))
            code = self.rfile.read(length).decode("utf-8")
            done = threading.Event()
            result = {}
            _jobs.put((code, result, done))
            done.wait(timeout=1800)
            body = json.dumps(result).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def log_message(self, *args):
            pass

    def _tick(_delta):
        while not _jobs.empty():
            code, result, done = _jobs.get()
            out = io.StringIO()
            try:
                with contextlib.redirect_stdout(out), contextlib.redirect_stderr(out):
                    exec(compile(code, "<bridge>", "exec"), _scope)
                result["ok"] = True
            except Exception:
                result["ok"] = False
                result["error"] = traceback.format_exc()
            result["output"] = out.getvalue()
            done.set()

    unreal.register_slate_post_tick_callback(_tick)
    _port = 8766 if "-game" in unreal.SystemLibrary.get_command_line().lower() else 8765
    _server = ThreadingHTTPServer(("127.0.0.1", _port), _Handler)
    threading.Thread(target=_server.serve_forever, daemon=True).start()
    unreal.log("Editor bridge listening on 127.0.0.1:%d" % _port)
