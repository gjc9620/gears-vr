using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

class WorkbenchManager : Form {
    readonly string root=AppDomain.CurrentDomain.BaseDirectory;
    readonly TextBox output=new TextBox { Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical,Dock=DockStyle.Fill };
    readonly FlowLayoutPanel buttons=new FlowLayoutPanel { Dock=DockStyle.Top,Height=96,AutoSize=false,Padding=new Padding(8) };
    bool busy;
    string Adb { get {return Path.Combine(root,"adb","adb.exe");} }
    static string Quote(string text) {return "\""+text.Replace("\"","\\\"")+"\"";}
    void Log(string text) {if(InvokeRequired){BeginInvoke(new Action<string>(Log),text);return;}output.AppendText(text+Environment.NewLine);}
    string Run(string file,string arguments) {
        using(var p=new Process()) {
            p.StartInfo=new ProcessStartInfo(file,arguments) { UseShellExecute=false,CreateNoWindow=true,RedirectStandardOutput=true,RedirectStandardError=true,StandardOutputEncoding=Encoding.UTF8,StandardErrorEncoding=Encoding.UTF8,WorkingDirectory=root };
            p.StartInfo.EnvironmentVariables["PYTHONUTF8"]="1";
            p.Start();var stdout=p.StandardOutput.ReadToEndAsync();var stderr=p.StandardError.ReadToEndAsync();
            if(!p.WaitForExit(120000))throw new Exception("操作仍在等待设备，请确认头显已连接、唤醒并允许 USB 调试。");
            Task.WaitAll(stdout,stderr);string result=stdout.Result+stderr.Result;if(p.ExitCode!=0)throw new Exception(result);return result;
        }
    }
    string Serial() {string text=Run(Adb,"devices");var rows=text.Split('\n').Select(x=>x.Trim().Split(new[]{' ','\t'},StringSplitOptions.RemoveEmptyEntries)).Where(x=>x.Length==2&&x[1]=="device").ToArray();if(rows.Length!=1)throw new Exception("请连接一台 Quest，并在头显里允许 USB 调试。当前可用设备数量："+rows.Length);return rows[0][0];}
    async void Action(string name,Func<string> operation) {if(busy)return;busy=true;buttons.Enabled=false;Log(name+"…");try {string result=await Task.Run(operation);Log(result);Log("完成。");}catch(Exception e){Log("未完成："+e.Message);}finally {busy=false;buttons.Enabled=true;}}
    void Add(string name,EventHandler handler) {var button=new Button {Text=name,AutoSize=true,Height=34,Margin=new Padding(4)};button.Click+=handler;buttons.Controls.Add(button);}
    public WorkbenchManager() {
        Text="GearsVR · Quest 武器工作台";Width=850;Height=590;MinimumSize=new Size(700,480);Font=new Font("Microsoft YaHei UI",10);
        var intro=new Label {Text="Quest 3 独立工作台：调整姿势和区域 → 手柄测试手感 → A/X 回编辑 → 保存设置",Dock=DockStyle.Top,Height=58,Padding=new Padding(12),AutoSize=false};
        Add("检查 Q3 连接",delegate {Action("检查连接",()=>Run(Adb,"devices -l"));});
        Add("安装 / 更新工作台",delegate {Action("安装工作台",()=>{string serial=Serial();return Run(Adb,"-s "+Quote(serial)+" install -r "+Quote(Path.Combine(root,"GearsVR-Quest-Workbench.apk")));});});
        Add("在 Q3 打开工作台",delegate {Action("启动工作台",()=>{string serial=Serial();return Run(Adb,"-s "+Quote(serial)+" shell am start -a android.intent.action.MAIN -c com.oculus.intent.category.VR -n com.gearsvr.workbench/android.app.NativeActivity");});});
        Add("导出姿势和测试设置",delegate {Action("从 Q3 导出已保存项目",()=>Run("py.exe","-3 "+Quote(Path.Combine(root,"tools","quest-workbench","transfer.py"))+" --adb "+Quote(Adb)+" pull --output "+Quote(Path.Combine(root,"exports"))));});
        Add("导入 GLB 模型",delegate {using(var dialog=new OpenFileDialog {Filter="自包含 GLB 模型 (*.glb)|*.glb"}){if(dialog.ShowDialog(this)!=DialogResult.OK)return;string model=dialog.FileName;Action("导入模型",()=>Run("py.exe","-3 "+Quote(Path.Combine(root,"tools","quest-workbench","transfer.py"))+" --adb "+Quote(Adb)+" import-glb "+Quote(model)));}});
        Add("打开导出目录",delegate {string path=Path.Combine(root,"exports");Directory.CreateDirectory(path);Process.Start("explorer.exe",Quote(path));});
        Add("查看使用说明",delegate {Process.Start("notepad.exe",Quote(Path.Combine(root,"使用说明.txt")));});
        Controls.Add(output);Controls.Add(buttons);Controls.Add(intro);
        Log("0.5.3 build17：可移动弹药、左轮整只装填弹巢、独立副手握持、精简部件、手柄测试与黄色引导。");
        Log("先选武器，再选交互部件；主面板只有采集左手、采集右手和大号测试按钮。高级设置可收起。");
        Log("左轮：拉出转轮 → 左肩取装填弹巢 → 送入 → 左手掌靠近转轮按一次Grip合上。只补原容量缺额与可用备用量。");
        Log("霰弹枪：左手握护木，右手大幅下压后抬回；换弹与每次射击后都需完成，不含装饰转枪。");
        Log("兽族步枪关锁扣即可射击；Snub空仓需拉套筒；狙击提拉推锁；Boomshot开膛装填；弓装箭后蓄力松扳机射。");
        Log("A/X回编辑后可调整再测试，B/Y重新放置。切换武器保持工作区朝向，不主动重新回中。");
        Log("裸手采集前在高级设置校准；一只手摆姿势，另一只手点击采集。每次自动另存独立条目。");
        Log("各武器的初始抓点与姿势是可编辑起点，仍需头显调校。新机制是工作台模拟；游戏接入和手感对照单独完成。");
        Log("导出保留完整项目和姿势库；旧Mod手指转换仍只用于Lancer，不会自动覆盖游戏。");
        FormClosing+=delegate(object sender,FormClosingEventArgs e){if(busy){e.Cancel=true;Log("设备操作进行中，请等本次操作完成后关闭。");}};
    }
    [STAThread] static void Main() {Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);Application.Run(new WorkbenchManager());}
}
