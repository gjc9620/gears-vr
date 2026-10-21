using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using UELib;
using UELib.Core;

class RecoverClasses {
    static int Main(string[] args) {
        if(args.Length!=3) return 2;
        Directory.CreateDirectory(args[1]);
        var selected=new HashSet<string>(File.ReadAllLines(args[2]));
        var pkg=UnrealLoader.LoadPackage(args[0],FileAccess.Read);
        pkg.InitializePackage(UnrealPackage.InitFlags.Construct | UnrealPackage.InitFlags.RegisterClasses);
        using(var errors=new StreamWriter(Path.Combine(args[1],"errors.txt"),false,Encoding.UTF8)) {
            int preloaded=0,failed=0,exported=0;
            foreach(var obj in pkg.Objects) {
                if((int)obj<=0 || (!(obj is UConst) && !(obj is UEnum))) continue;
                try {obj.BeginDeserializing();preloaded++;}
                catch(Exception ex) {errors.WriteLine(obj.GetReferencePath()+": preload: "+ex);failed++;}
            }
            if(failed!=0) return 3;
            foreach(var obj in pkg.Objects) {
                if(!(obj is UClass) || !selected.Contains(obj.GetReferencePath())) continue;
                try {
                    obj.BeginDeserializing();
                    File.WriteAllText(Path.Combine(args[1],Regex.Replace(obj.GetReferencePath(),"[^a-zA-Z0-9_.-]","_")+".uc"),obj.Decompile(),Encoding.UTF8);
                    exported++;
                } catch(Exception ex) {errors.WriteLine(obj.GetReferencePath()+": "+ex);}
            }
            Console.WriteLine("preloaded="+preloaded+"; exported="+exported+"; selected="+selected.Count);
        }
        return 0;
    }
}
