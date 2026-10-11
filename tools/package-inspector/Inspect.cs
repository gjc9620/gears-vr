using System;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using UELib;

class Inspect
{
    static int Main(string[] args)
    {
        if (args.Length != 2 && args.Length != 3) { Console.Error.WriteLine("Usage: Inspect.exe package output-directory [reference-path-regex]"); return 2; }
        Directory.CreateDirectory(args[1]);
        try
        {
            var pkg = UnrealLoader.LoadPackage(args[0], FileAccess.Read);
            Console.WriteLine("Parsed exports: " + pkg.Exports.Count);
            if (pkg.Summary.CompressedChunks != null && pkg.Summary.CompressionFlags != 0)
            {
                using (var chunks = new StreamWriter(Path.Combine(args[1], "chunks.tsv")))
                {
                    chunks.WriteLine("UncompressedOffset\tCompressedOffset\tUncompressedSize\tCompressedSize");
                    foreach (var chunk in pkg.Summary.CompressedChunks)
                        chunks.WriteLine("{0}\t{1}\t{2}\t{3}", chunk.UncompressedOffset, chunk.CompressedOffset, chunk.UncompressedSize, chunk.CompressedSize);
                }
                Console.Error.WriteLine("Compressed package: decompress before object export.");
                return 3;
            }
            pkg.InitializePackage(UnrealPackage.InitFlags.Construct | UnrealPackage.InitFlags.RegisterClasses);
            using (var index = new StreamWriter(Path.Combine(args[1], "objects.tsv"), false, Encoding.UTF8))
            using (var errors = new StreamWriter(Path.Combine(args[1], "errors.txt"), false, Encoding.UTF8))
            {
                index.WriteLine("Index\tClass\tPath");
                int exported = 0;
                foreach (var obj in pkg.Objects)
                {
                    if ((int)obj <= 0) continue;
                    string path = obj.GetReferencePath();
                    string cls = obj is UELib.Core.UClass ? "Class" : obj.Class == null ? "" : obj.Class.Name.ToString();
                    index.WriteLine("{0}\t{1}\t{2}", (int)obj, cls, path);
                    bool candidate = cls == "Class" && Regex.IsMatch(path, "Camera|PlayerController|LocalPlayer|Viewport", RegexOptions.IgnoreCase);
                    candidate |= cls == "Function" && Regex.IsMatch(path, "(WarPlayerCamera|Camera)\\.|GetPlayerViewPoint|CalcCamera|UpdateCamera", RegexOptions.IgnoreCase);
                    if (args.Length == 3) candidate = Regex.IsMatch(path, args[2], RegexOptions.IgnoreCase);
                    if (!candidate) continue;
                    try
                    {
                        obj.BeginDeserializing();
                        File.WriteAllText(Path.Combine(args[1], Regex.Replace(path, "[^a-zA-Z0-9_.-]", "_") + ".uc"), obj.Decompile(), Encoding.UTF8);
                        ++exported;
                    }
                    catch (Exception ex) { errors.WriteLine(path + ": " + ex); }
                }
                Console.WriteLine("Candidate classes exported: " + exported);
            }
            return 0;
        }
        catch (Exception ex) { Console.Error.WriteLine(ex); return 1; }
    }
}
