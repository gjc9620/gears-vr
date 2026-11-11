using System;
using System.IO;
using System.Collections.Generic;
using System.Reflection;
using System.Web.Script.Serialization;
using UELib;
using UELib.Core;

class DescribeShapeCache {
    static object Value(object value, string member, int depth) {
        if(value == null) return null;
        var obj = value as UObject;
        if(obj != null) {
            if(depth < 2 && (member.IndexOf("Inner",StringComparison.OrdinalIgnoreCase)>=0 || member=="Struct"))
                return Describe(obj,depth+1);
            return new { index=(int)obj, reference=obj.GetReferencePath() };
        }
        var type=value.GetType();
        return type.IsPrimitive || type.IsEnum || value is string ? value.ToString() : "<"+type.FullName+">";
    }
    static object Describe(UObject obj,int depth) {
        obj.BeginDeserializing();
        var members=new Dictionary<string,object>();
        foreach(var f in obj.GetType().GetFields(BindingFlags.Public|BindingFlags.Instance))
            members["field:"+f.Name]=Value(f.GetValue(obj),f.Name,depth);
        foreach(var p in obj.GetType().GetProperties(BindingFlags.Public|BindingFlags.Instance)) {
            if(!p.CanRead || p.GetIndexParameters().Length!=0)continue;
            try { members["property:"+p.Name]=Value(p.GetValue(obj,null),p.Name,depth); }
            catch(Exception error) { members["property:"+p.Name]="ERROR:"+error.GetType().FullName; }
        }
        return new { index=(int)obj, reference=obj.GetReferencePath(), implementation=obj.GetType().FullName, members=members };
    }
    static int Main(string[] args) {
        if(args.Length!=1)return 2;
        try {
            var package=UnrealLoader.LoadPackage(args[0],FileAccess.Read);
            package.InitializePackage(UnrealPackage.InitFlags.Construct|UnrealPackage.InitFlags.RegisterClasses);
            var rows=new List<object>();
            foreach(var obj in package.Objects)
                if(((int)obj==23202 || (int)obj==23203))rows.Add(Describe(obj,0));
            if(rows.Count!=2)throw new InvalidDataException("Two original cache arrays required");
            Console.WriteLine(new JavaScriptSerializer().Serialize(new {
                scope="Original serialized package metadata parsed read-only; not live native PropertySize, alignment or stride", arrays=rows }));
            return 0;
        } catch(Exception error) { Console.Error.WriteLine(error);return 1; }
    }
}
