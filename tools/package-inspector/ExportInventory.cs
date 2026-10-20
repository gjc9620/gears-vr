// Read-only serialized-export inventory; never deserialize or execute scripts.
using System;
using System.IO;
using System.Text;
using System.Security.Cryptography;
using UELib;

class ExportInventory
{
    static int Main(string[] args)
    {
        if (args.Length != 2) return 2;
        try
        {
            var pkg = UnrealLoader.LoadPackage(args[0], FileAccess.Read);
            if (pkg.Summary.CompressionFlags != 0) throw new InvalidDataException("Decompress an analysis copy first");
            using (var names = new StreamWriter(args[1] + ".names.tsv", false, Encoding.UTF8))
            {
                names.WriteLine("Index\tName");
                foreach (var name in pkg.Names) names.WriteLine("{0}\t{1}", name.Index, name.Name);
            }
            pkg.InitializePackage(UnrealPackage.InitFlags.Construct | UnrealPackage.InitFlags.RegisterClasses);
            using (var file = File.OpenRead(args[0]))
            using (var hash = SHA256.Create())
            using (var output = new StreamWriter(args[1], false, Encoding.UTF8))
            {
                output.WriteLine("Index\tClass\tPath\tOffset\tSize\tSHA256\tObjectFlags");
                foreach (var obj in pkg.Objects)
                {
                    if ((int)obj <= 0) continue;
                    var entry = obj.ExportTable;
                    if (entry.SerialOffset < 0 || entry.SerialSize < 0 || entry.SerialOffset + (long)entry.SerialSize > file.Length)
                        throw new InvalidDataException("Invalid export bounds");
                    var bytes = new byte[entry.SerialSize];
                    file.Position = entry.SerialOffset;
                    int read = 0;
                    while (read < bytes.Length)
                    {
                        int count = file.Read(bytes, read, bytes.Length - read);
                        if (count == 0) throw new EndOfStreamException();
                        read += count;
                    }
                    string cls = obj is UELib.Core.UClass ? "Class" : obj.Class == null ? "" : obj.Class.Name.ToString();
                    output.WriteLine("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}", (int)obj, cls,
                        obj.GetReferencePath(), entry.SerialOffset, entry.SerialSize,
                        BitConverter.ToString(hash.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant(), entry.ObjectFlags);
                }
            }
            return 0;
        }
        catch (Exception error) { Console.Error.WriteLine(error); return 1; }
    }
}
