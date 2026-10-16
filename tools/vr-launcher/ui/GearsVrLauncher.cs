using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;

namespace GearsVrUi {
    static class Program {
        [STAThread] static int Main(string[] args) {
            bool selfTest = args.Length == 1 && args[0] == "--self-test";
            try {
                string root = FindRoot(AppDomain.CurrentDomain.BaseDirectory);
                if (selfTest) return SelfTest(root);
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new Launcher(root)); return 0;
            } catch (Exception e) { if (selfTest) Console.Error.WriteLine(e); else MessageBox.Show(e.Message, "Gears VR 启动失败"); return 1; }
        }
        internal static string FindRoot(string start) {
            for (DirectoryInfo d = new DirectoryInfo(start); d != null; d = d.Parent)
                if (File.Exists(Path.Combine(d.FullName, "tools", "vr-launcher", "launcher.py"))) return d.FullName;
            throw new IOException("找不到仓库。请从仓库 analysis/vr-launcher-ui 目录运行生成的启动器。");
        }
        internal static string Quote(string value) {
            var b = new StringBuilder("\""); int slash = 0;
            foreach (char c in value) {
                if (c == '\\') { slash++; continue; }
                if (c == '"') b.Append('\\', slash * 2 + 1).Append(c);
                else b.Append('\\', slash).Append(c);
                slash = 0;
            }
            return b.Append('\\', slash * 2).Append('"').ToString();
        }
        internal static string Arguments(string root, string command, string runtime) {
            string result = Quote(Path.Combine(root, "tools", "vr-launcher", "launcher.py")) + " " + command;
            if (command == "start") {
                result += " --manifest " + Quote(Path.Combine(root, "tools", "vr-launcher", "manifest.json"));
                if (!String.IsNullOrEmpty(runtime)) result += " --runtime " + Quote(runtime);
            }
            return result;
        }
        internal static Dictionary<string, object> ReadConfig(string path) {
            return new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(File.ReadAllText(path));
        }
        internal static void SaveGame(string path, string game) {
            SaveOptions(path, game, false, 65);
        }
        internal static bool SupportsHudSize(Dictionary<string, object> manifest) {
            object value;
            if (!manifest.TryGetValue("hud_size_control", out value)) return false;
            if (!(value is bool)) throw new InvalidDataException("manifest 的 hud_size_control 必须是布尔值。");
            return (bool)value;
        }
        internal static decimal HudSize(Dictionary<string, object> config) {
            object value;
            if (!config.TryGetValue("hud_horizontal_fov_degrees", out value)) return 65;
            if (!(value is int || value is long || value is decimal || value is double || value is float))
                throw new InvalidDataException("HUD 大小必须是 45–100° 的数字。");
            double number = Convert.ToDouble(value);
            if (Double.IsNaN(number) || Double.IsInfinity(number) || number < 45 || number > 100)
                throw new InvalidDataException("HUD 大小必须在 45–100° 之间。");
            return Convert.ToDecimal(value);
        }
        internal static void SaveOptions(string path, string game, bool hudSupported, decimal hudSize) {
            if (hudSupported && (hudSize < 45 || hudSize > 100)) throw new ArgumentOutOfRangeException("hudSize");
            var config = ReadConfig(path); config["game_path"] = game;
            if (hudSupported) config["hud_horizontal_fov_degrees"] = hudSize;
            string temporary = path + ".ui.tmp";
            try {
                File.WriteAllText(temporary, new JavaScriptSerializer().Serialize(config), new UTF8Encoding(false));
                File.Replace(temporary, path, null);
            } finally { if (File.Exists(temporary)) File.Delete(temporary); }
        }
        internal static string Phase(string json) {
            var value = new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(json);
            object phase;
            if (value.TryGetValue("session_phase", out phase) && phase != null) return phase.ToString();
            object launcher;
            if (value.TryGetValue("launcher", out launcher)) {
                var indexed = launcher as Dictionary<string, object>;
                if (indexed != null && indexed.TryGetValue("phase", out phase)) return Convert.ToString(phase);
            }
            return "未知";
        }
        internal static bool OwnIndexReady(string json, int pid, long creation) {
            var value = new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(json);
            object kind, identity;
            if (!value.TryGetValue("kind", out kind) || Convert.ToString(kind) != "repository_vr_launcher" ||
                !value.TryGetValue("owner_identity", out identity)) return false;
            var owner = identity as Dictionary<string, object>; object p, c;
            return owner != null && owner.TryGetValue("pid", out p) && owner.TryGetValue("creation_time", out c) &&
                Convert.ToInt32(p) == pid && Convert.ToInt64(c) == creation;
        }
        static int SelfTest(string root) {
            string args = Arguments(root, "start", "C:\\runtime with spaces\\openxr.json");
            if (!args.Contains("--manifest") || !args.Contains("--runtime") || args.Contains("auto-continue")) return 2;
            if (Arguments(root, "stop", null).Contains("--runtime")) return 3;
            if (Arguments(root, "recenter", "unused").Contains("--runtime") ||
                !Arguments(root, "recenter", null).EndsWith(" recenter")) return 18;
            if (Quote("a b\\") != "\"a b\\\\\"" || Quote("a\"b") != "\"a\\\"b\"") return 4;
            if (Phase("{\"launcher\":{\"phase\":\"preflight\"}}") != "preflight" ||
                Phase("{\"session_phase\":\"running\",\"launcher\":{\"phase\":\"old\"}}") != "running") return 5;
            const string ready = "{\"kind\":\"repository_vr_launcher\",\"owner_identity\":{\"pid\":123,\"creation_time\":456}}";
            if (OwnIndexReady("{}",123,456) || OwnIndexReady(ready,124,456) || OwnIndexReady(ready,123,457) ||
                !OwnIndexReady(ready,123,456)) return 7;
            string temp = Path.GetTempFileName();
            try {
                File.WriteAllText(temp, "{\"game_path\":\"old\",\"stereo_units_per_metre\":100,\"extra\":{\"keep\":true}}");
                SaveGame(temp, "C:\\game path\\game.exe"); var config = ReadConfig(temp);
                if (!config.ContainsKey("extra") || Convert.ToInt32(config["stereo_units_per_metre"]) != 100 ||
                    Convert.ToString(config["game_path"]) != "C:\\game path\\game.exe") return 6;
                if (config.ContainsKey("hud_horizontal_fov_degrees") || HudSize(config) != 65) return 8;
                if (SupportsHudSize(new Dictionary<string, object>()) || SupportsHudSize(new Dictionary<string, object>{{"hud_size_control", false}}) ||
                    !SupportsHudSize(new Dictionary<string, object>{{"hud_size_control", true}})) return 9;
                foreach (object invalid in new object[]{"true", 1, null}) {
                    try { SupportsHudSize(new Dictionary<string, object>{{"hud_size_control", invalid}}); return 10; }
                    catch (InvalidDataException) { }
                }
                SaveOptions(temp, "new-game", true, 62.5m); config = ReadConfig(temp);
                if (HudSize(config) != 62.5m || Convert.ToString(config["game_path"]) != "new-game" ||
                    !((Dictionary<string, object>)config["extra"]).ContainsKey("keep") || Convert.ToInt32(config["stereo_units_per_metre"]) != 100) return 11;
                SaveGame(temp, "other-game"); if (HudSize(ReadConfig(temp)) != 62.5m) return 12;
                foreach (decimal size in new decimal[]{45, 100}) { SaveOptions(temp, "game", true, size); if (HudSize(ReadConfig(temp)) != size) return 13; }
                foreach (decimal size in new decimal[]{44.9m, 100.1m}) {
                    string before = File.ReadAllText(temp);
                    try { SaveOptions(temp, "bad", true, size); return 14; } catch (ArgumentOutOfRangeException) { }
                    if (File.ReadAllText(temp) != before) return 15;
                }
                foreach (object invalid in new object[]{true, "65", null, Double.NaN, Double.PositiveInfinity, 44.9, 100.1}) {
                    try { HudSize(new Dictionary<string, object>{{"hud_horizontal_fov_degrees", invalid}}); return 16; }
                    catch (InvalidDataException) { }
                }
                File.WriteAllText(temp, "{\"game_path\":\"old\",\"unknown\":42}");
                SaveOptions(temp, "legacy", false, 65); config = ReadConfig(temp);
                if (config.ContainsKey("hud_horizontal_fov_degrees") || Convert.ToInt32(config["unknown"]) != 42) return 17;
            } finally { File.Delete(temp); }
            var python = Python.Find();
            File.WriteAllText(Path.Combine(root, "analysis", "vr-launcher-ui", "self-test.txt"),
                "PASS: repository, quoting, start/stop arguments, phase parsing, config preservation, HUD capability/range/save/readback/legacy behavior, owner identity handshake, Python probe\r\nPython: " + python.File);
            return 0;
        }
    }
    class Python {
        internal string File, Prefix;
        static string Resolve(string name) {
            if (Path.IsPathRooted(name)) return System.IO.File.Exists(name) ? name : null;
            foreach (string dir in (Environment.GetEnvironmentVariable("PATH") ?? "").Split(';')) {
                try { string path = Path.Combine(dir.Trim('"'), name); if (System.IO.File.Exists(path)) return path; } catch { }
            }
            return null;
        }
        internal static Python Find() {
            string home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
            string preferred = Environment.GetEnvironmentVariable("GEARSVR_PYTHON");
            var candidates = new List<string>();
            if (!String.IsNullOrWhiteSpace(preferred)) candidates.Add(Environment.ExpandEnvironmentVariables(preferred.Trim('"')));
            candidates.Add(Path.Combine(home, ".cache", "codex-runtimes", "codex-primary-runtime", "dependencies", "python", "python.exe"));
            candidates.Add("py.exe"); candidates.Add("python.exe"); candidates.Add("python3.exe");
            foreach (string candidate in candidates) {
                string file = Resolve(candidate); if (file == null) continue;
                string prefix = Path.GetFileName(file).Equals("py.exe", StringComparison.OrdinalIgnoreCase) ? "-3 " : "";
                try {
                    using (var process = new Process()) {
                        process.StartInfo = new ProcessStartInfo(file, prefix + "-c " + Program.Quote("import sys; print(sys.executable); print(sys.version_info[0])")) {
                            UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden,
                            RedirectStandardOutput = true, RedirectStandardError = true };
                        process.Start(); var output = process.StandardOutput.ReadToEndAsync(); var error = process.StandardError.ReadToEndAsync();
                        if (!process.WaitForExit(5000)) { process.Kill(); continue; } // Only the disposable Python locator probe.
                        string[] lines = output.Result.Trim().Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
                        // py.exe is only a locator: launch python.exe directly so
                        // its PID/creation time is the actual indexed owner identity.
                        if (process.ExitCode == 0 && lines.Length == 2 && lines[1] == "3" && System.IO.File.Exists(lines[0]))
                            return new Python { File = lines[0], Prefix = "" };
                    }
                } catch { }
            }
            throw new IOException("未找到可用 Python 3。请将 GEARSVR_PYTHON 设置为 python.exe 完整路径。");
        }
    }
    class Launcher : Form {
        readonly string root, config, logPath;
        readonly object logLock = new object();
        TextBox game = new TextBox(), runtime = new TextBox(), output = new TextBox();
        ComboBox runtimeMode = new ComboBox();
        Button start = new Button(), stop = new Button(), recenter = new Button(), status = new Button(), browseGame = new Button(), browseRuntime = new Button();
        Label state = new Label();
        NumericUpDown hudSize = new NumericUpDown { Minimum = 45, Maximum = 100, Value = 65, DecimalPlaces = 2, Increment = 1, Width = 85 };
        bool hudSupported;
        Process owner;
        int ownerPid; long ownerCreation;
        volatile bool stopRequested;
        bool starting, stopping, recentering, closingAfterOwner;
        Python python;
        public Launcher(string repository) {
            root = repository; config = Path.Combine(root, "tools", "vr-launcher", "config.json");
            logPath = Path.Combine(root, "analysis", "runtime", "vr-launcher-ui.log");
            Directory.CreateDirectory(Path.GetDirectoryName(logPath));
            hudSupported = Program.SupportsHudSize(Program.ReadConfig(Path.Combine(root, "tools", "vr-launcher", "manifest.json")));
            Text = "Gears VR 实验启动器 v0.1"; ClientSize = new Size(820, 570); MinimumSize = new Size(740, 520);
            Font = new Font("Microsoft YaHei UI", 9F); StartPosition = FormStartPosition.CenterScreen;
            var layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(14), ColumnCount = 3, RowCount = 9 };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 90)); layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100)); layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 90));
            layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 62));
            for (int i = 1; i < 4; i++) layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 38));
            layout.RowStyles.Add(new RowStyle(SizeType.Absolute, hudSupported ? 38 : 0));
            for (int i = 5; i < 7; i++) layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 38));
            layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100)); layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 28));
            var info = new Label { Text = "实验版本 · 跟随游戏当前分辨率渲染\r\nDLSS / DLAA 已禁用。手动进入关卡；不会自动结束测试。", Dock = DockStyle.Fill };
            layout.Controls.Add(info, 0, 0); layout.SetColumnSpan(info, 3);
            layout.Controls.Add(new Label { Text = "游戏路径", AutoSize = true }, 0, 1); game.Dock = DockStyle.Fill; layout.Controls.Add(game, 1, 1);
            browseGame.Text = "浏览…"; layout.Controls.Add(browseGame, 2, 1);
            layout.Controls.Add(new Label { Text = "OpenXR", AutoSize = true }, 0, 2);
            runtimeMode.DropDownStyle = ComboBoxStyle.DropDownList; runtimeMode.Items.AddRange(new object[] { "本地 OpenXR 模拟器", "系统 OpenXR Runtime" }); runtimeMode.SelectedIndex = 0; runtimeMode.Dock = DockStyle.Fill;
            layout.Controls.Add(runtimeMode, 1, 2);
            layout.Controls.Add(new Label { Text = "模拟器文件", AutoSize = true }, 0, 3); runtime.Dock = DockStyle.Fill; layout.Controls.Add(runtime, 1, 3);
            browseRuntime.Text = "浏览…"; layout.Controls.Add(browseRuntime, 2, 3);
            runtime.Text = Path.Combine(root, "analysis", "runtime", "simulator-quiet2", "openxr_simulator.json");
            if (hudSupported) {
                layout.Controls.Add(new Label { Text = "HUD 大小", AutoSize = true }, 0, 4);
                var hud = new FlowLayoutPanel { Dock = DockStyle.Fill, WrapContents = false };
                hud.Controls.Add(hudSize);
                hud.Controls.Add(new Label { Text = "°  调大更易读 / 调小更集中；下次启动生效", AutoSize = true, Padding = new Padding(0, 5, 0, 0) });
                layout.Controls.Add(hud, 1, 4); layout.SetColumnSpan(hud, 2);
            }
            var buttons = new FlowLayoutPanel { Dock = DockStyle.Fill };
            start.Text = "启动 VR"; stop.Text = "停止 VR"; recenter.Text = "视角回中"; status.Text = "查看状态";
            var logs = new Button { Text = "打开日志", AutoSize = true };
            foreach (var button in new[] { start, stop, recenter, status, logs }) { button.AutoSize = true; buttons.Controls.Add(button); }
            layout.Controls.Add(buttons, 0, 5); layout.SetColumnSpan(buttons, 3);
            state.Text = "就绪"; state.Dock = DockStyle.Fill; layout.Controls.Add(state, 0, 6); layout.SetColumnSpan(state, 3);
            output.Multiline = true; output.ReadOnly = true; output.ScrollBars = ScrollBars.Both; output.WordWrap = false; output.Dock = DockStyle.Fill;
            layout.Controls.Add(output, 0, 7); layout.SetColumnSpan(output, 3);
            var footer = new Label { Text = "停止仅请求现有启动器正常清理，不强制关闭游戏。", Dock = DockStyle.Fill };
            layout.Controls.Add(footer, 0, 8); layout.SetColumnSpan(footer, 3); Controls.Add(layout);
            try { var saved = Program.ReadConfig(config); game.Text = Convert.ToString(saved["game_path"]); if (hudSupported) hudSize.Value = Program.HudSize(saved); } catch (Exception e) { Log("读取配置失败：" + e.Message); }
            browseGame.Click += delegate { using (var dialog = new OpenFileDialog { Filter = "游戏程序 (*.exe)|*.exe" }) if (dialog.ShowDialog(this) == DialogResult.OK) game.Text = dialog.FileName; };
            browseRuntime.Click += delegate { using (var dialog = new OpenFileDialog { Filter = "OpenXR manifest (*.json)|*.json" }) if (dialog.ShowDialog(this) == DialogResult.OK) runtime.Text = dialog.FileName; };
            runtimeMode.SelectedIndexChanged += delegate { UpdateButtons(); };
            start.Click += async delegate { await StartVr(); };
            stop.Click += async delegate { await StopVr(); };
            recenter.Click += async delegate { await RecenterVr(); };
            status.Click += async delegate { status.Enabled = false; await Command("status", null); status.Enabled = true; };
            logs.Click += delegate { try { Process.Start("explorer.exe", Program.Quote(Path.GetDirectoryName(logPath))); } catch (Exception e) { Log(e.Message); } };
            FormClosing += OnClosingRequested; UpdateButtons();
        }
        void Ui(Action action) { if (!IsDisposed && IsHandleCreated) try { BeginInvoke(action); } catch (InvalidOperationException) { } }
        void Log(string message) {
            string line = DateTime.Now.ToString("HH:mm:ss") + " " + message + Environment.NewLine;
            lock (logLock) { try { File.AppendAllText(logPath, line, Encoding.UTF8); } catch { } }
            Ui(delegate { if (output.TextLength > 100000) output.Text = output.Text.Substring(output.TextLength - 60000); output.AppendText(line); });
        }
        void UpdateButtons() {
            bool busy = starting || owner != null;
            start.Enabled = !busy && !stopping; stop.Enabled = !stopping;
            recenter.Enabled = owner != null && !stopping && !recentering && !stopRequested;
            game.Enabled = browseGame.Enabled = runtimeMode.Enabled = !busy;
            hudSize.Enabled = !busy;
            runtime.Enabled = browseRuntime.Enabled = !busy && runtimeMode.SelectedIndex == 0;
        }
        async Task RecenterVr() {
            if (owner == null || stopping || recentering || stopRequested) return;
            if (!OwnerReady()) { Log("VR 尚未就绪，暂时无法回中。"); return; }
            recentering = true; UpdateButtons();
            try {
                int code = await Command("recenter", null);
                if (owner != null && !stopping && !stopRequested)
                    state.Text = code == 0 ? "已请求视角回中" : "回中未完成，请查看日志";
            } finally { recentering = false; UpdateButtons(); }
        }
        async Task StartVr() {
            if (starting || owner != null) return;
            try {
                string path = game.Text.Trim(); string resolved = Path.IsPathRooted(path) ? path : Path.Combine(root, path);
                if (!File.Exists(resolved)) throw new IOException("游戏程序不存在，请选择正确路径。");
                string manifest = Path.Combine(root, "tools", "vr-launcher", "manifest.json");
                if (!File.Exists(manifest)) throw new IOException("缺少 manifest.json，请先完成此实验构建。");
                string selectedRuntime = runtimeMode.SelectedIndex == 0 ? runtime.Text.Trim() : null;
                if (selectedRuntime != null && !Path.IsPathRooted(selectedRuntime)) selectedRuntime = Path.GetFullPath(Path.Combine(root, selectedRuntime));
                if (selectedRuntime != null && !File.Exists(selectedRuntime)) throw new IOException("模拟器 manifest 不存在，请浏览选择 JSON 文件。");
                bool currentHudSupport = Program.SupportsHudSize(Program.ReadConfig(manifest));
                if (currentHudSupport != hudSupported) throw new IOException("启动清单能力已变化，请重新打开启动器。");
                Program.SaveOptions(config, path, hudSupported, hudSize.Value); stopRequested = false; starting = true; UpdateButtons(); state.Text = "启动中：请手动进入游戏关卡";
                int result = await Command("start", selectedRuntime);
                if (result < 0) state.Text = "启动失败，请查看下方日志";
            } catch (Exception e) { Log(e.Message); state.Text = "启动失败：" + e.Message; }
            finally { starting = false; UpdateButtons(); if (closingAfterOwner && owner == null) { closingAfterOwner = false; Close(); } }
        }
        async Task StopVr() {
            if (stopping) return; stopping = true; stopRequested = true; UpdateButtons();
            bool ownAttempt = starting || owner != null;
            state.Text = "正在取消启动，请稍候…";
            if (ownAttempt) {
                // launcher.py writes this identity only after creating its stop
                // event. Ignore stale index files and remember early requests.
                while ((starting || owner != null) && !OwnerReady()) await Task.Delay(200);
                if (!starting && owner == null) {
                    stopping = false; state.Text = "启动已取消或 VR 会话已结束"; UpdateButtons(); return;
                }
            }
            state.Text = "正在请求停止 VR…";
            int code = await Command("stop", null); stopping = false; UpdateButtons();
            if (code != 0) { state.Text = "停止未完成，请查看日志；窗口保留以便重试"; closingAfterOwner = false; }
            else if (owner != null) state.Text = "已收到停止请求，正在等待 VR 会话结束";
        }
        bool OwnerReady() {
            if (owner == null) return false;
            try { return Program.OwnIndexReady(File.ReadAllText(Path.Combine(root, "analysis", "runtime", "vr-launcher-session.json")), ownerPid, ownerCreation); }
            catch { return false; }
        }
        async Task<int> Command(string command, string selectedRuntime) {
            try {
                int code = await Task.Run(delegate {
                    if (python == null) python = Python.Find();
                    using (var process = new Process()) {
                        process.StartInfo = new ProcessStartInfo(python.File, python.Prefix + Program.Arguments(root, command, selectedRuntime)) {
                            WorkingDirectory = root, UseShellExecute = false, CreateNoWindow = true,
                            WindowStyle = ProcessWindowStyle.Hidden, RedirectStandardOutput = true, RedirectStandardError = true,
                            StandardOutputEncoding = Encoding.UTF8, StandardErrorEncoding = Encoding.UTF8 };
                        process.StartInfo.EnvironmentVariables["PYTHONUNBUFFERED"] = "1";
                        process.StartInfo.EnvironmentVariables["PYTHONIOENCODING"] = "utf-8";
                        if (command == "start" && selectedRuntime == null) process.StartInfo.EnvironmentVariables.Remove("XR_RUNTIME_JSON");
                        var stdout = new StringBuilder();
                        process.OutputDataReceived += delegate(object sender, DataReceivedEventArgs e) { if (e.Data != null) { if (command == "status") stdout.AppendLine(e.Data); Log(e.Data); } };
                        process.ErrorDataReceived += delegate(object sender, DataReceivedEventArgs e) { if (e.Data != null) Log(e.Data); };
                        Log("执行 launcher.py " + command);
                        if (command == "start" && stopRequested) { Log("启动前已收到停止请求，取消本次启动"); return 0; }
                        process.Start();
                        if (command == "start") {
                            int pid = process.Id; long creation = process.StartTime.ToUniversalTime().ToFileTimeUtc();
                            Ui(delegate { owner = process; ownerPid = pid; ownerCreation = creation; UpdateButtons(); });
                        }
                        process.BeginOutputReadLine(); process.BeginErrorReadLine(); process.WaitForExit();
                        int exit = process.ExitCode;
                        if (command == "status" && exit == 0) {
                            try { string phase = Program.Phase(stdout.ToString()); Ui(delegate { state.Text = "当前阶段：" + phase; }); } catch { }
                        }
                        if (command == "start") Ui(delegate { owner = null; state.Text = exit == 0 ? "VR 会话已结束" : "VR 会话已退出，请查看下方日志（退出码 " + exit + "）"; UpdateButtons(); });
                        Log(command + " 结束，退出码 " + exit); return exit;
                    }
                });
                return code;
            } catch (Exception e) { Log(command + " 失败：" + e.Message); return -1; }
        }
        async void OnClosingRequested(object sender, FormClosingEventArgs e) {
            if (!starting && owner == null) return;
            e.Cancel = true;
            if (closingAfterOwner) return;
            if (MessageBox.Show(this, "VR 仍在启动或运行。是否停止 VR，等待清理完成后关闭此窗口？\r\n选择“否”继续运行。不会强制结束游戏。", "停止 VR", MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;
            closingAfterOwner = true; await StopVr();
            if (!starting && owner == null && closingAfterOwner) { closingAfterOwner = false; Close(); }
        }
    }
}
