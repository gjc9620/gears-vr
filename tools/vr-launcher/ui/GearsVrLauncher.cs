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
    enum TextKey { StartupError, RepositoryMissing, HudCapabilityInvalid, HudNumberInvalid, HudRangeInvalid, Unknown, PythonMissing, WindowTitle, Intro, GamePath, Browse, SystemRuntime, Simulator, RuntimeFile, HudSize, HudHint, StartVr, StopVr, Recenter, Status, OpenLogs, Ready, StopHint, ConfigReadError, GameFilter, RecenterNotReady, RecenterRequested, RecenterFailed, GameMissing, ManifestMissing, RuntimeMissing, ManifestChanged, Starting, StartupFailed, StartupFailedPrefix, Cancelling, Cancelled, Stopping, StopFailed, WaitingForStop, RunningCommand, CancelBeforeLaunch, CurrentPhase, SessionEnded, SessionExit, ExitCode, CommandFailed, ConfirmStop, CameraDistance, CameraHint, CameraInvalid, ExportLogs, ExportingLogs, LogsExported, ExportLogsFailed, ExportFilter, ExportInvalidResult, ExportWarnings }
    static class Language {
        internal static volatile bool English = true;
        internal static readonly string[] Chinese = new string[] {
            "Gears VR 启动失败",
            "找不到仓库。请从仓库 analysis/vr-launcher-ui 目录运行生成的启动器。",
            "manifest 的 hud_size_control 必须是布尔值。",
            "HUD 大小必须是 45–100° 的数字。",
            "HUD 大小必须在 45–100° 之间。",
            "未知",
            "未找到可用 Python 3。请将 GEARSVR_PYTHON 设置为 python.exe 完整路径。",
            "Gears VR 实验启动器 v0.1",
            "实验版本 · 跟随游戏当前分辨率渲染\r\nDLSS / DLAA 已禁用。手动进入关卡；不会自动结束测试。",
            "游戏路径",
            "浏览…",
            "系统 OpenXR Runtime",
            "本地 OpenXR 模拟器",
            "Runtime 文件",
            "HUD 大小",
            "°  调大更易读 / 调小更集中；下次启动生效",
            "启动 VR",
            "停止 VR",
            "视角回中",
            "查看状态",
            "打开日志",
            "就绪",
            "停止仅请求现有启动器正常清理，不强制关闭游戏。",
            "读取配置失败：",
            "游戏程序 (*.exe)|*.exe",
            "VR 尚未就绪，暂时无法回中。",
            "已请求视角回中",
            "回中未完成，请查看日志",
            "游戏程序不存在，请选择正确路径。",
            "缺少 manifest.json，请先完成此实验构建。",
            "所选 Runtime 文件不存在。VDXR 请先安装 Virtual Desktop Streamer，或浏览选择正确的 JSON 文件。",
            "启动清单能力已变化，请重新打开启动器。",
            "启动中：请手动进入游戏关卡",
            "启动失败，请查看下方日志",
            "启动失败：",
            "正在取消启动，请稍候…",
            "启动已取消或 VR 会话已结束",
            "正在请求停止 VR…",
            "停止未完成，请查看日志；窗口保留以便重试",
            "已收到停止请求，正在等待 VR 会话结束",
            "执行 launcher.py ",
            "启动前已收到停止请求，取消本次启动",
            "当前阶段：",
            "VR 会话已结束",
            "VR 会话已退出，请查看下方日志（退出码 ",
            " 结束，退出码 ",
            " 失败：",
            "VR 仍在启动或运行。是否停止 VR，等待清理完成后关闭此窗口？\r\n选择“否”继续运行。不会强制结束游戏。",
            "镜头拉近",
            "实验：米（0–0.5）；0 为原镜头。普通游戏中生效，下次启动应用。",
            "镜头拉近必须为 0–0.5 米的数字；构建能力必须为布尔值。",
            "导出日志",
            "正在导出日志…",
            "日志已导出：",
            "日志导出失败，请重试或查看下方详情：",
            "诊断日志 ZIP (*.zip)|*.zip",
            "导出程序未返回有效的 ZIP 保存回执。",
            "导出提示：",
        };
        internal static readonly string[] Translations = new string[] {
            "Gears VR startup failed",
            "Repository not found. Run the launcher from the repository or analysis/vr-launcher-ui.",
            "The manifest hud_size_control must be a boolean.",
            "HUD size must be a number from 45 to 100 degrees.",
            "HUD size must be between 45 and 100 degrees.",
            "Unknown",
            "Python 3 not found. Set GEARSVR_PYTHON to the full python.exe path.",
            "Gears VR Experimental Launcher v0.1",
            "Experimental build - Uses the current game resolution\r\nDLSS / DLAA disabled. Enter a level manually; no automatic timeout.",
            "Game path",
            "Browse...",
            "System OpenXR runtime",
            "Local OpenXR simulator",
            "Runtime file",
            "HUD size",
            "Degrees. Larger: easier to read / smaller: more focused; applies next launch",
            "Start VR",
            "Stop VR",
            "Recenter",
            "Status",
            "Open logs",
            "Ready",
            "Stop requests normal launcher cleanup without forcibly closing the game.",
            "Cannot read configuration: ",
            "Game executable (*.exe)|*.exe",
            "VR is not ready to recenter yet.",
            "Recenter requested",
            "Recenter failed; check the log",
            "Game executable not found. Select a valid path.",
            "manifest.json is missing. Complete the experimental build first.",
            "Runtime file not found. For VDXR, install Virtual Desktop Streamer or select the correct JSON file.",
            "Manifest capabilities changed. Reopen the launcher.",
            "Starting: enter a game level manually",
            "Startup failed; check the log below",
            "Startup failed: ",
            "Cancelling startup; please wait...",
            "Startup cancelled or VR session ended",
            "Requesting VR stop...",
            "Stop failed; check the log and retry in this window",
            "Stop requested; waiting for the VR session to end",
            "Running launcher.py ",
            "Stop requested before launch; cancelling startup",
            "Current phase: ",
            "VR session ended",
            "VR session exited; check the log below (exit code ",
            " finished, exit code ",
            " failed: ",
            "VR is still starting or running. Stop VR and close this window after cleanup?\r\nChoose No to keep running. The game will not be forcibly terminated.",
            "Camera closer",
            "Experimental: metres (0–0.5); 0 = original. Gameplay only; next launch.",
            "Camera closer must be a number from 0 to 0.5 metres; build capability must be boolean.",
            "Export logs",
            "Exporting logs...",
            "Logs exported: ",
            "Could not export logs. Retry or check the details below: ",
            "Diagnostic logs ZIP (*.zip)|*.zip",
            "The exporter did not return a valid ZIP save receipt.",
            "Export note: ",
        };
        internal static string T(TextKey key) { return English ? Translations[(int)key] : Chinese[(int)key]; }
        internal static string TranslateVisible(string text, bool english) {
            var source = english ? Chinese : Translations; var target = english ? Translations : Chinese;
            for (int i = 0; i < source.Length; i++) if (text == source[i]) return target[i];
            // A failure can include a localized exception after its status prefix.
            int failure = (int)TextKey.StartupFailedPrefix;
            if (text.StartsWith(source[failure], StringComparison.Ordinal))
                return target[failure] + TranslateVisible(text.Substring(source[failure].Length), english);
            int best = -1;
            for (int i = 0; i < source.Length; i++)
                if (text.StartsWith(source[i], StringComparison.Ordinal) && (best < 0 || source[i].Length > source[best].Length)) best = i;
            return best < 0 ? text : target[best] + text.Substring(source[best].Length);
        }
    }
    static class Program {
        [STAThread] static int Main(string[] args) {
            bool selfTest = args.Length == 1 && args[0] == "--self-test";
            try {
                string root = FindRoot(AppDomain.CurrentDomain.BaseDirectory);
                if (selfTest) return SelfTest(root);
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new Launcher(root)); return 0;
            } catch (Exception e) { if (selfTest) Console.Error.WriteLine(e); else MessageBox.Show(e.Message, Language.T(TextKey.StartupError)); return 1; }
        }
        internal static string FindRoot(string start) {
            for (DirectoryInfo d = new DirectoryInfo(start); d != null; d = d.Parent)
                if (File.Exists(Path.Combine(d.FullName, "tools", "vr-launcher", "launcher.py"))) return d.FullName;
            throw new IOException(Language.T(TextKey.RepositoryMissing));
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
                result += " --foreground --manifest " + Quote(Path.Combine(root, "tools", "vr-launcher", "manifest.json"));
                if (!String.IsNullOrEmpty(runtime)) result += " --runtime " + Quote(runtime);
            }
            return result;
        }
        internal static string ExportArguments(string root, string destination, bool overwrite) {
            return Arguments(root, "export-logs", null) + " --output " + Quote(destination) + (overwrite ? " --overwrite" : "");
        }
        internal static Dictionary<string, object> ExportReceipt(string json, string destination) {
            var result = new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(json);
            object success, saved;
            if (result == null || !result.TryGetValue("diagnostics_exported", out success) ||
                !(success is bool) || !(bool)success || !result.TryGetValue("output", out saved) ||
                !(saved is string) || !Path.IsPathRooted((string)saved) ||
                !String.Equals(Path.GetFullPath((string)saved), Path.GetFullPath(destination), StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException(Language.T(TextKey.ExportInvalidResult));
            return result;
        }
        internal static string DefaultRuntime(string root, int mode) {
            if (mode == 0) return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Virtual Desktop Streamer", "OpenXR", "virtualdesktop-openxr.json");
            if (mode == 1) return "";
            if (mode == 2) return Path.Combine(root, "analysis", "runtime", "simulator-quiet2", "openxr_simulator.json");
            throw new ArgumentOutOfRangeException("mode");
        }
        internal static Dictionary<string, object> ReadConfig(string path) {
            return new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(File.ReadAllText(path));
        }
        internal static bool IsEnglish(Dictionary<string, object> config) {
            object language;
            return !config.TryGetValue("launcher_language", out language) || Convert.ToString(language) != "zh-CN";
        }
        internal static void SaveLanguage(string path, bool english) {
            var config = ReadConfig(path); config["launcher_language"] = english ? "en" : "zh-CN";
            WriteConfig(path, config);
        }
        internal static void SaveGame(string path, string game) {
            SaveOptions(path, game, false, 65);
        }
        internal static bool SupportsHudSize(Dictionary<string, object> manifest) {
            object value;
            if (!manifest.TryGetValue("hud_size_control", out value)) return false;
            if (!(value is bool)) throw new InvalidDataException(Language.T(TextKey.HudCapabilityInvalid));
            return (bool)value;
        }
        internal static decimal HudSize(Dictionary<string, object> config) {
            object value;
            if (!config.TryGetValue("hud_horizontal_fov_degrees", out value)) return 65;
            if (!(value is int || value is long || value is decimal || value is double || value is float))
                throw new InvalidDataException(Language.T(TextKey.HudNumberInvalid));
            double number = Convert.ToDouble(value);
            if (Double.IsNaN(number) || Double.IsInfinity(number) || number < 45 || number > 100)
                throw new InvalidDataException(Language.T(TextKey.HudRangeInvalid));
            return Convert.ToDecimal(value);
        }
        internal static bool SupportsCameraDistance(Dictionary<string, object> manifest) {
            object value;
            if (!manifest.TryGetValue("camera_distance_control", out value)) return false;
            if (!(value is bool)) throw new InvalidDataException(Language.T(TextKey.CameraInvalid));
            return (bool)value;
        }
        internal static decimal CameraCloser(Dictionary<string, object> config) {
            object value;
            if (!config.TryGetValue("camera_closer_metres", out value)) return 0;
            if (!(value is int || value is long || value is decimal || value is double || value is float))
                throw new InvalidDataException(Language.T(TextKey.CameraInvalid));
            double number = Convert.ToDouble(value);
            if (Double.IsNaN(number) || Double.IsInfinity(number) || number < 0 || number > .5)
                throw new InvalidDataException(Language.T(TextKey.CameraInvalid));
            return Convert.ToDecimal(value);
        }
        internal static void SaveOptions(string path, string game, bool hudSupported, decimal hudSize) {
            SaveOptions(path, game, hudSupported, hudSize, false, 0);
        }
        internal static void SaveOptions(string path, string game, bool hudSupported, decimal hudSize, bool cameraSupported, decimal cameraCloser) {
            if (cameraSupported && (cameraCloser < 0 || cameraCloser > .5m)) throw new ArgumentOutOfRangeException("cameraCloser");
            if (hudSupported && (hudSize < 45 || hudSize > 100)) throw new ArgumentOutOfRangeException("hudSize");
            var config = ReadConfig(path); config["game_path"] = game;
            if (hudSupported) config["hud_horizontal_fov_degrees"] = hudSize;
            if (cameraSupported) config["camera_closer_metres"] = cameraCloser;
            WriteConfig(path, config);
        }
        static void WriteConfig(string path, Dictionary<string, object> config) {
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
            return Language.T(TextKey.Unknown);
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
            if (Language.Chinese.Length != Enum.GetValues(typeof(TextKey)).Length || Language.Translations.Length != Language.Chinese.Length) return 33;
            if (!DefaultRuntime(root, 0).EndsWith("virtualdesktop-openxr.json") || DefaultRuntime(root, 1) != "" || !DefaultRuntime(root, 2).EndsWith("openxr_simulator.json")) return 20;
            string args = Arguments(root, "start", "C:\\runtime with spaces\\openxr.json");
            if (!args.Contains("--foreground") || !args.Contains("--manifest") || !args.Contains("--runtime") || args.Contains("auto-continue")) return 2;
            if (Arguments(root, "stop", null).Contains("--runtime")) return 3;
            if (Arguments(root, "recenter", "unused").Contains("--runtime") ||
                !Arguments(root, "recenter", null).EndsWith(" recenter")) return 18;
            if (Quote("a b\\") != "\"a b\\\\\"" || Quote("a\"b") != "\"a\\\"b\"") return 4;
            if (ExportArguments(@"C:\Root folder", @"C:\Feedback folder\run.zip", false) !=
                "\"C:\\Root folder\\tools\\vr-launcher\\launcher.py\" export-logs --output \"C:\\Feedback folder\\run.zip\"") return 34;
            if (ExportArguments(@"C:\Root folder", "C:\\Feedback folder\\run \"quote\".zip", true) !=
                "\"C:\\Root folder\\tools\\vr-launcher\\launcher.py\" export-logs --output \"C:\\Feedback folder\\run \\\"quote\\\".zip\" --overwrite") return 35;
            const string exportDestination = @"C:\Feedback folder\run.zip";
            var exportJson = new JavaScriptSerializer();
            ExportReceipt(exportJson.Serialize(new Dictionary<string, object>{{"diagnostics_exported", true}, {"output", exportDestination}}), exportDestination);
            foreach (object invalid in new object[]{false, "true", null}) {
                try { ExportReceipt(exportJson.Serialize(new Dictionary<string, object>{{"diagnostics_exported", invalid}, {"output", exportDestination}}), exportDestination); return 36; }
                catch (InvalidDataException) { }
            }
            foreach (string invalid in new string[]{"run.zip", @"C:\Feedback folder\different.zip"}) {
                try { ExportReceipt(exportJson.Serialize(new Dictionary<string, object>{{"diagnostics_exported", true}, {"output", invalid}}), exportDestination); return 37; }
                catch (InvalidDataException) { }
            }
            if (Phase("{\"launcher\":{\"phase\":\"preflight\"}}") != "preflight" ||
                Phase("{\"session_phase\":\"running\",\"launcher\":{\"phase\":\"old\"}}") != "running") return 5;
            const string ready = "{\"kind\":\"repository_vr_launcher\",\"owner_identity\":{\"pid\":123,\"creation_time\":456}}";
            if (OwnIndexReady("{}",123,456) || OwnIndexReady(ready,124,456) || OwnIndexReady(ready,123,457) ||
                !OwnIndexReady(ready,123,456)) return 7;
            if (!Language.English || !IsEnglish(new Dictionary<string, object>())) return 31;
            string temp = Path.GetTempFileName();
            try {
                File.WriteAllText(temp, "{\"game_path\":\"old\",\"stereo_units_per_metre\":100,\"extra\":{\"keep\":true}}");
                SaveGame(temp, "C:\\game path\\game.exe"); var config = ReadConfig(temp);
                if (!config.ContainsKey("extra") || Convert.ToInt32(config["stereo_units_per_metre"]) != 100 ||
                    Convert.ToString(config["game_path"]) != "C:\\game path\\game.exe") return 6;
                if (config.ContainsKey("hud_horizontal_fov_degrees") || HudSize(config) != 65) return 8;
                if (!IsEnglish(config) || config.ContainsKey("launcher_language")) return 32;
                if (SupportsHudSize(new Dictionary<string, object>()) || SupportsHudSize(new Dictionary<string, object>{{"hud_size_control", false}}) ||
                    !SupportsHudSize(new Dictionary<string, object>{{"hud_size_control", true}})) return 9;
                foreach (object invalid in new object[]{"true", 1, null}) {
                    try { SupportsHudSize(new Dictionary<string, object>{{"hud_size_control", invalid}}); return 10; }
                    catch (InvalidDataException) { }
                }
                SaveOptions(temp, "new-game", true, 62.5m); config = ReadConfig(temp);
                if (HudSize(config) != 62.5m || Convert.ToString(config["game_path"]) != "new-game" ||
                    !((Dictionary<string, object>)config["extra"]).ContainsKey("keep") || Convert.ToInt32(config["stereo_units_per_metre"]) != 100) return 11;
                SaveLanguage(temp, true); config = ReadConfig(temp);
                if (!IsEnglish(config) || HudSize(config) != 62.5m || !config.ContainsKey("extra")) return 21;
                SaveLanguage(temp, false);
                if (IsEnglish(ReadConfig(temp))) return 22;
                Language.English = true;
                if (Language.T(TextKey.ExportLogs) != "Export logs" || Language.Chinese[(int)TextKey.ExportLogs] != "导出日志") return 38;
                if (Language.T(TextKey.StartVr) != "Start VR" || Language.TranslateVisible(Language.Chinese[34] + "detail", true) != "Startup failed: detail") return 23;
                for (int i = 0; i < Language.Chinese.Length; i++)
                    if (Language.TranslateVisible(Language.Chinese[i], true) != Language.Translations[i] || Language.TranslateVisible(Language.Translations[i], false) != Language.Chinese[i]) return 24;
                Language.English = false;
                SaveGame(temp, "other-game");
                if (Convert.ToString(ReadConfig(temp)["launcher_language"]) != "zh-CN") return 25; if (HudSize(ReadConfig(temp)) != 62.5m) return 12;
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
                if (CameraCloser(config) != 0 || SupportsCameraDistance(new Dictionary<string, object>()) ||
                    !SupportsCameraDistance(new Dictionary<string, object>{{"camera_distance_control", true}})) return 26;
                SaveOptions(temp, "closer-game", false, 65, true, .3m); config = ReadConfig(temp);
                if (CameraCloser(config) != .3m || Convert.ToInt32(config["unknown"]) != 42) return 27;
                SaveGame(temp, "preserve-camera");
                if (CameraCloser(ReadConfig(temp)) != .3m) return 28;
                foreach (object invalid in new object[]{true, "0.3", null, Double.NaN, Double.PositiveInfinity, -.01, .51}) {
                    try { CameraCloser(new Dictionary<string, object>{{"camera_closer_metres", invalid}}); return 29; }
                    catch (InvalidDataException) { }
                }
                foreach (decimal distance in new decimal[]{0, .5m}) {
                    SaveOptions(temp, "bounds", false, 65, true, distance);
                    if (CameraCloser(ReadConfig(temp)) != distance) return 30;
                }
            } finally { File.Delete(temp); }
            string home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
            var candidates = Python.Candidates(root, "\"C:\\Python With Spaces\\python.exe\"", home);
            if (candidates[0] != "C:\\Python With Spaces\\python.exe" ||
                candidates[1] != Path.Combine(root, "runtime", "python", "python.exe") ||
                candidates[2] != Path.Combine(home, ".cache", "codex-runtimes", "codex-primary-runtime", "dependencies", "python", "python.exe") ||
                candidates[3] != "py.exe") return 18;
            if (Python.Candidates(root, null, home)[0] != candidates[1]) return 19;
            var python = Python.Find(root);
            string previousPython = Environment.GetEnvironmentVariable("GEARSVR_PYTHON");
            try {
                Environment.SetEnvironmentVariable("GEARSVR_PYTHON", python.File);
                if (!String.Equals(Python.Find(root).File, python.File, StringComparison.OrdinalIgnoreCase)) return 20;
            } finally { Environment.SetEnvironmentVariable("GEARSVR_PYTHON", previousPython); }
            File.WriteAllText(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "self-test.txt"),
                "PASS: repository, quoting, start/stop/export arguments, export receipt validation, phase parsing, config preservation, HUD capability/range/save/readback/legacy behavior, bilingual catalog/language persistence, owner identity handshake, Python probe\r\nPython: " + python.File);
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
        internal static List<string> Candidates(string root, string preferred, string home) {
            var candidates = new List<string>();
            if (!String.IsNullOrWhiteSpace(preferred)) candidates.Add(Environment.ExpandEnvironmentVariables(preferred.Trim().Trim('"')));
            candidates.Add(Path.Combine(root, "runtime", "python", "python.exe"));
            candidates.Add(Path.Combine(home, ".cache", "codex-runtimes", "codex-primary-runtime", "dependencies", "python", "python.exe"));
            candidates.Add("py.exe"); candidates.Add("python.exe"); candidates.Add("python3.exe");
            return candidates;
        }
        internal static Python Find(string root) {
            string home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
            foreach (string candidate in Candidates(root, Environment.GetEnvironmentVariable("GEARSVR_PYTHON"), home)) {
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
            throw new IOException(Language.T(TextKey.PythonMissing));
        }
    }
    class Launcher : Form {
        readonly string root, config, logPath;
        readonly object logLock = new object();
        TextBox game = new TextBox(), runtime = new TextBox(), output = new TextBox();
        ComboBox runtimeMode = new ComboBox(), languageMode = new ComboBox();
        Button start = new Button(), stop = new Button(), recenter = new Button(), status = new Button(), browseGame = new Button(), browseRuntime = new Button(), exportLogs = new Button();
        Label state = new Label();
        NumericUpDown hudSize = new NumericUpDown { Minimum = 45, Maximum = 100, Value = 65, DecimalPlaces = 2, Increment = 1, Width = 85 };
        NumericUpDown cameraCloser = new NumericUpDown { Minimum = 0, Maximum = .5m, Value = 0, DecimalPlaces = 2, Increment = .05m, Width = 85 };
        bool hudSupported, cameraSupported;
        Process owner;
        int ownerPid; long ownerCreation;
        volatile bool stopRequested;
        bool starting, stopping, recentering, closingAfterOwner, translating, exportBusy;
        Python python;
        public Launcher(string repository) {
            root = repository; config = Path.Combine(root, "tools", "vr-launcher", "config.json");
            logPath = Path.Combine(root, "analysis", "runtime", "vr-launcher-ui.log");
            Directory.CreateDirectory(Path.GetDirectoryName(logPath));
            try { Language.English = Program.IsEnglish(Program.ReadConfig(config)); } catch { }
            hudSupported = Program.SupportsHudSize(Program.ReadConfig(Path.Combine(root, "tools", "vr-launcher", "manifest.json")));
            cameraSupported = Program.SupportsCameraDistance(Program.ReadConfig(Path.Combine(root, "tools", "vr-launcher", "manifest.json")));
            Text = Language.T(TextKey.WindowTitle); ClientSize = new Size(960, 610); MinimumSize = new Size(940, 520);
            Font = new Font("Microsoft YaHei UI", 9F); StartPosition = FormStartPosition.CenterScreen;
            var layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(14), ColumnCount = 3, RowCount = 10 };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 110)); layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100)); layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 110));
            layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 62));
            for (int i = 1; i < 4; i++) layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 38));
            layout.RowStyles.Add(new RowStyle(SizeType.Absolute, hudSupported ? 38 : 0));
            layout.RowStyles.Add(new RowStyle(SizeType.Absolute, cameraSupported ? 38 : 0));
            for (int i = 6; i < 8; i++) layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 38));
            layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100)); layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 28));
            var info = new Label { Text = Language.T(TextKey.Intro), Dock = DockStyle.Fill };
            layout.Controls.Add(info, 0, 0); layout.SetColumnSpan(info, 2);
            languageMode.DropDownStyle = ComboBoxStyle.DropDownList;
            languageMode.Items.AddRange(new object[] { "\u4e2d\u6587", "English" });
            languageMode.SelectedIndex = Language.English ? 1 : 0;
            languageMode.AccessibleName = "Language / \u8bed\u8a00"; languageMode.Dock = DockStyle.Top;
            layout.Controls.Add(languageMode, 2, 0);
            layout.Controls.Add(new Label { Text = Language.T(TextKey.GamePath), AutoSize = true }, 0, 1); game.Dock = DockStyle.Fill; layout.Controls.Add(game, 1, 1);
            browseGame.Text = Language.T(TextKey.Browse); layout.Controls.Add(browseGame, 2, 1);
            layout.Controls.Add(new Label { Text = "OpenXR", AutoSize = true }, 0, 2);
            runtimeMode.DropDownStyle = ComboBoxStyle.DropDownList; runtimeMode.Items.AddRange(new object[] { "Quest 3 / Virtual Desktop (VDXR)", Language.T(TextKey.SystemRuntime), Language.T(TextKey.Simulator) }); runtimeMode.SelectedIndex = 0; runtimeMode.Dock = DockStyle.Fill;
            layout.Controls.Add(runtimeMode, 1, 2);
            layout.Controls.Add(new Label { Text = Language.T(TextKey.RuntimeFile), AutoSize = true }, 0, 3); runtime.Dock = DockStyle.Fill; layout.Controls.Add(runtime, 1, 3);
            browseRuntime.Text = Language.T(TextKey.Browse); layout.Controls.Add(browseRuntime, 2, 3);
            runtime.Text = Program.DefaultRuntime(root, 0);
            if (hudSupported) {
                layout.Controls.Add(new Label { Text = Language.T(TextKey.HudSize), AutoSize = true }, 0, 4);
                var hud = new FlowLayoutPanel { Dock = DockStyle.Fill, WrapContents = false };
                hud.Controls.Add(hudSize);
                hud.Controls.Add(new Label { Text = Language.T(TextKey.HudHint), AutoSize = true, Padding = new Padding(0, 5, 0, 0) });
                layout.Controls.Add(hud, 1, 4); layout.SetColumnSpan(hud, 2);
            }
            if (cameraSupported) {
                layout.Controls.Add(new Label { Text = Language.T(TextKey.CameraDistance), AutoSize = true }, 0, 5);
                var camera = new FlowLayoutPanel { Dock = DockStyle.Fill, WrapContents = false };
                camera.Controls.Add(cameraCloser);
                camera.Controls.Add(new Label { Text = Language.T(TextKey.CameraHint), AutoSize = true, Padding = new Padding(0, 5, 0, 0) });
                layout.Controls.Add(camera, 1, 5); layout.SetColumnSpan(camera, 2);
            }
            var buttons = new FlowLayoutPanel { Dock = DockStyle.Fill };
            start.Text = Language.T(TextKey.StartVr); stop.Text = Language.T(TextKey.StopVr); recenter.Text = Language.T(TextKey.Recenter); status.Text = Language.T(TextKey.Status);
            var logs = new Button { Text = Language.T(TextKey.OpenLogs), AutoSize = true };
            exportLogs.Text = Language.T(TextKey.ExportLogs);
            foreach (var button in new[] { start, stop, recenter, status, logs, exportLogs }) { button.AutoSize = true; buttons.Controls.Add(button); }
            layout.Controls.Add(buttons, 0, 6); layout.SetColumnSpan(buttons, 3);
            state.Text = Language.T(TextKey.Ready); state.Dock = DockStyle.Fill; layout.Controls.Add(state, 0, 7); layout.SetColumnSpan(state, 3);
            output.Multiline = true; output.ReadOnly = true; output.ScrollBars = ScrollBars.Both; output.WordWrap = false; output.Dock = DockStyle.Fill;
            layout.Controls.Add(output, 0, 8); layout.SetColumnSpan(output, 3);
            var footer = new Label { Text = Language.T(TextKey.StopHint), Dock = DockStyle.Fill };
            layout.Controls.Add(footer, 0, 9); layout.SetColumnSpan(footer, 3); Controls.Add(layout);
            try { var saved = Program.ReadConfig(config); game.Text = Convert.ToString(saved["game_path"]); if (hudSupported) hudSize.Value = Program.HudSize(saved); if (cameraSupported) cameraCloser.Value = Program.CameraCloser(saved); } catch (Exception e) { Log(Language.T(TextKey.ConfigReadError) + e.Message); }
            browseGame.Click += delegate { using (var dialog = new OpenFileDialog { Filter = Language.T(TextKey.GameFilter) }) if (dialog.ShowDialog(this) == DialogResult.OK) game.Text = dialog.FileName; };
            browseRuntime.Click += delegate { using (var dialog = new OpenFileDialog { Filter = "OpenXR manifest (*.json)|*.json" }) if (dialog.ShowDialog(this) == DialogResult.OK) runtime.Text = dialog.FileName; };
            runtimeMode.SelectedIndexChanged += delegate { if (translating) return; runtime.Text = Program.DefaultRuntime(root, runtimeMode.SelectedIndex); UpdateButtons(); };
            start.Click += async delegate { await StartVr(); };
            stop.Click += async delegate { await StopVr(); };
            recenter.Click += async delegate { await RecenterVr(); };
            status.Click += async delegate { status.Enabled = false; await Command("status", null); status.Enabled = true; };
            logs.Click += delegate { try { Process.Start("explorer.exe", Program.Quote(Path.GetDirectoryName(logPath))); } catch (Exception e) { Log(e.Message); } };
            exportLogs.Click += async delegate { await ExportLogs(); };
            languageMode.SelectedIndexChanged += delegate {
                bool english = languageMode.SelectedIndex == 1;
                if (english == Language.English) return;
                try { Program.SaveLanguage(config, english); }
                catch (Exception e) {
                    languageMode.SelectedIndex = Language.English ? 1 : 0;
                    MessageBox.Show(this, e.Message, Text, MessageBoxButtons.OK, MessageBoxIcon.Error); return;
                }
                Language.English = english; TranslateControls(this, english);
                translating = true;
                try { runtimeMode.Items[1] = Language.T(TextKey.SystemRuntime); runtimeMode.Items[2] = Language.T(TextKey.Simulator); }
                finally { translating = false; }
            };
            FormClosing += OnClosingRequested; UpdateButtons();
        }
        void TranslateControls(Control control, bool english) {
            if (control != output && control != game && control != runtime && control != languageMode && control != runtimeMode)
                control.Text = Language.TranslateVisible(control.Text, english);
            foreach (Control child in control.Controls) TranslateControls(child, english);
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
            hudSize.Enabled = cameraCloser.Enabled = !busy;
            runtime.Enabled = browseRuntime.Enabled = !busy && runtimeMode.SelectedIndex != 1;
            exportLogs.Enabled = !exportBusy;
        }
        async Task ExportLogs() {
            if (exportBusy) return;
            exportBusy = true; UpdateButtons();
            try {
                string destination;
                bool overwrite;
                using (var dialog = new SaveFileDialog {
                    Title = Language.T(TextKey.ExportLogs), Filter = Language.T(TextKey.ExportFilter),
                    FileName = "GearsVR-Diagnostics-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".zip",
                    DefaultExt = "zip", AddExtension = true, OverwritePrompt = true, CheckPathExists = true }) {
                    if (dialog.ShowDialog(this) != DialogResult.OK) return;
                    destination = Path.GetFullPath(dialog.FileName);
                    overwrite = File.Exists(destination); // Save dialog confirmed replacing an existing choice.
                }
                state.Text = Language.T(TextKey.ExportingLogs);
                Log(Language.T(TextKey.ExportingLogs));
                var result = await Task.Run(delegate {
                    var selectedPython = python ?? Python.Find(root);
                    using (var process = new Process()) {
                        process.StartInfo = new ProcessStartInfo(selectedPython.File,
                            selectedPython.Prefix + Program.ExportArguments(root, destination, overwrite)) {
                            WorkingDirectory = root, UseShellExecute = false, CreateNoWindow = true,
                            WindowStyle = ProcessWindowStyle.Hidden, RedirectStandardOutput = true, RedirectStandardError = true,
                            StandardOutputEncoding = Encoding.UTF8, StandardErrorEncoding = Encoding.UTF8 };
                        process.StartInfo.EnvironmentVariables["PYTHONUNBUFFERED"] = "1";
                        process.StartInfo.EnvironmentVariables["PYTHONIOENCODING"] = "utf-8";
                        process.StartInfo.EnvironmentVariables["GEARSVR_PYTHON"] = selectedPython.File;
                        process.Start();
                        var stdout = process.StandardOutput.ReadToEndAsync();
                        var stderr = process.StandardError.ReadToEndAsync();
                        process.WaitForExit(); Task.WaitAll(stdout, stderr);
                        if (process.ExitCode != 0)
                            throw new IOException("Exit " + process.ExitCode + ": " + stderr.Result + stdout.Result);
                        if (!String.IsNullOrWhiteSpace(stderr.Result)) Log(stderr.Result.Trim());
                        var receipt = Program.ExportReceipt(stdout.Result, destination);
                        if (!File.Exists(destination)) throw new IOException(Language.T(TextKey.ExportInvalidResult));
                        return receipt;
                    }
                });
                string saved = (string)result["output"];
                Log(Language.T(TextKey.LogsExported) + saved);
                object warnings;
                if (result.TryGetValue("warnings", out warnings) && warnings is System.Collections.IEnumerable && !(warnings is string))
                    foreach (object warning in (System.Collections.IEnumerable)warnings) Log(Language.T(TextKey.ExportWarnings) + Convert.ToString(warning));
                state.Text = Language.T(TextKey.LogsExported) + saved;
            } catch (Exception e) {
                Log(Language.T(TextKey.ExportLogsFailed) + e.Message);
                state.Text = Language.T(TextKey.ExportLogsFailed) + e.Message;
            } finally { exportBusy = false; UpdateButtons(); }
        }
        async Task RecenterVr() {
            if (owner == null || stopping || recentering || stopRequested) return;
            if (!OwnerReady()) { Log(Language.T(TextKey.RecenterNotReady)); return; }
            recentering = true; UpdateButtons();
            try {
                int code = await Command("recenter", null);
                if (owner != null && !stopping && !stopRequested)
                    state.Text = code == 0 ? Language.T(TextKey.RecenterRequested) : Language.T(TextKey.RecenterFailed);
            } finally { recentering = false; UpdateButtons(); }
        }
        async Task StartVr() {
            if (starting || owner != null) return;
            try {
                string path = game.Text.Trim(); string resolved = Path.IsPathRooted(path) ? path : Path.Combine(root, path);
                if (!File.Exists(resolved)) throw new IOException(Language.T(TextKey.GameMissing));
                string manifest = Path.Combine(root, "tools", "vr-launcher", "manifest.json");
                if (!File.Exists(manifest)) throw new IOException(Language.T(TextKey.ManifestMissing));
                string selectedRuntime = runtimeMode.SelectedIndex != 1 ? runtime.Text.Trim() : null;
                if (selectedRuntime != null && !Path.IsPathRooted(selectedRuntime)) selectedRuntime = Path.GetFullPath(Path.Combine(root, selectedRuntime));
                if (selectedRuntime != null && !File.Exists(selectedRuntime)) throw new IOException(Language.T(TextKey.RuntimeMissing));
                bool currentHudSupport = Program.SupportsHudSize(Program.ReadConfig(manifest));
                if (currentHudSupport != hudSupported || Program.SupportsCameraDistance(Program.ReadConfig(manifest)) != cameraSupported) throw new IOException(Language.T(TextKey.ManifestChanged));
                Program.SaveOptions(config, path, hudSupported, hudSize.Value, cameraSupported, cameraCloser.Value); stopRequested = false; starting = true; UpdateButtons(); state.Text = Language.T(TextKey.Starting);
                int result = await Command("start", selectedRuntime);
                if (result < 0) state.Text = Language.T(TextKey.StartupFailed);
            } catch (Exception e) { Log(e.Message); state.Text = Language.T(TextKey.StartupFailedPrefix) + e.Message; }
            finally { starting = false; UpdateButtons(); if (closingAfterOwner && owner == null) { closingAfterOwner = false; Close(); } }
        }
        async Task StopVr() {
            if (stopping) return; stopping = true; stopRequested = true; UpdateButtons();
            bool ownAttempt = starting || owner != null;
            state.Text = Language.T(TextKey.Cancelling);
            if (ownAttempt) {
                // launcher.py writes this identity only after creating its stop
                // event. Ignore stale index files and remember early requests.
                while ((starting || owner != null) && !OwnerReady()) await Task.Delay(200);
                if (!starting && owner == null) {
                    stopping = false; state.Text = Language.T(TextKey.Cancelled); UpdateButtons(); return;
                }
            }
            state.Text = Language.T(TextKey.Stopping);
            int code = await Command("stop", null); stopping = false; UpdateButtons();
            if (code != 0) { state.Text = Language.T(TextKey.StopFailed); closingAfterOwner = false; }
            else if (owner != null) state.Text = Language.T(TextKey.WaitingForStop);
        }
        bool OwnerReady() {
            if (owner == null) return false;
            try { return Program.OwnIndexReady(File.ReadAllText(Path.Combine(root, "analysis", "runtime", "vr-launcher-session.json")), ownerPid, ownerCreation); }
            catch { return false; }
        }
        async Task<int> Command(string command, string selectedRuntime) {
            try {
                int code = await Task.Run(delegate {
                    if (python == null) python = Python.Find(root);
                    using (var process = new Process()) {
                        process.StartInfo = new ProcessStartInfo(python.File, python.Prefix + Program.Arguments(root, command, selectedRuntime)) {
                            WorkingDirectory = root, UseShellExecute = false, CreateNoWindow = true,
                            WindowStyle = ProcessWindowStyle.Hidden, RedirectStandardOutput = true, RedirectStandardError = true,
                            StandardOutputEncoding = Encoding.UTF8, StandardErrorEncoding = Encoding.UTF8 };
                        process.StartInfo.EnvironmentVariables["PYTHONUNBUFFERED"] = "1";
                        process.StartInfo.EnvironmentVariables["PYTHONIOENCODING"] = "utf-8";
                        process.StartInfo.EnvironmentVariables["GEARSVR_PYTHON"] = python.File;
                        if (command == "start" && selectedRuntime == null) process.StartInfo.EnvironmentVariables.Remove("XR_RUNTIME_JSON");
                        var stdout = new StringBuilder();
                        process.OutputDataReceived += delegate(object sender, DataReceivedEventArgs e) { if (e.Data != null) { if (command == "status") stdout.AppendLine(e.Data); Log(e.Data); } };
                        process.ErrorDataReceived += delegate(object sender, DataReceivedEventArgs e) { if (e.Data != null) Log(e.Data); };
                        Log(Language.T(TextKey.RunningCommand) + command);
                        if (command == "start" && stopRequested) { Log(Language.T(TextKey.CancelBeforeLaunch)); return 0; }
                        process.Start();
                        if (command == "start") {
                            int pid = process.Id; long creation = process.StartTime.ToUniversalTime().ToFileTimeUtc();
                            Ui(delegate { owner = process; ownerPid = pid; ownerCreation = creation; UpdateButtons(); });
                        }
                        process.BeginOutputReadLine(); process.BeginErrorReadLine(); process.WaitForExit();
                        int exit = process.ExitCode;
                        if (command == "status" && exit == 0) {
                            try { string phase = Program.Phase(stdout.ToString()); Ui(delegate { state.Text = Language.T(TextKey.CurrentPhase) + phase; }); } catch { }
                        }
                        if (command == "start") Ui(delegate { owner = null; state.Text = exit == 0 ? Language.T(TextKey.SessionEnded) : Language.T(TextKey.SessionExit) + exit + (Language.English ? ")" : "）"); UpdateButtons(); });
                        Log(command + Language.T(TextKey.ExitCode) + exit); return exit;
                    }
                });
                return code;
            } catch (Exception e) { Log(command + Language.T(TextKey.CommandFailed) + e.Message); return -1; }
        }
        async void OnClosingRequested(object sender, FormClosingEventArgs e) {
            if (!starting && owner == null) return;
            e.Cancel = true;
            if (closingAfterOwner) return;
            if (MessageBox.Show(this, Language.T(TextKey.ConfirmStop), Language.T(TextKey.StopVr), MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;
            closingAfterOwner = true; await StopVr();
            if (!starting && owner == null && closingAfterOwner) { closingAfterOwner = false; Close(); }
        }
    }
}
