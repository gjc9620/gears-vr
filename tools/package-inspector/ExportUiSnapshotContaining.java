// Read-only export of existing containing functions from a runtime snapshot.
// @category GearsVR
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.io.*;
public class ExportUiSnapshotContaining extends GhidraScript {
 public void run() throws Exception {
  if (!currentProgram.getName().startsWith("section_")) throw new IllegalArgumentException("Runtime snapshot only");
  String[] args=getScriptArgs(); Path out=Paths.get(args[0]); Files.createDirectories(out);
  DecompInterface d=new DecompInterface(); d.openProgram(currentProgram);
  try(PrintWriter idx=new PrintWriter(Files.newBufferedWriter(out.resolve("index.tsv")))) {
   idx.println("query\tentry\tbytes\tresult");
   for(int n=1;n<args.length;n++) {
    var a=toAddr(args[n]); Function f=getFunctionContaining(a);
    if(f==null) {idx.println(a+"\tunknown"); continue;}
    var r=d.decompileFunction(f,45,monitor); String entry=f.getEntryPoint().toString();
    idx.println(a+"\t"+entry+"\t"+f.getBody().getNumAddresses()+"\t"+r.decompileCompleted()+" "+r.getErrorMessage());
    if(r.decompileCompleted()) Files.writeString(out.resolve(entry+".c"),r.getDecompiledFunction().getC());
    try(PrintWriter p=new PrintWriter(Files.newBufferedWriter(out.resolve(entry+".asm")))) {
     var it=currentProgram.getListing().getInstructions(f.getBody(),true);
     while(it.hasNext()) {var i=it.next(); p.println(i.getAddress()+" "+i);}
    }
    try(PrintWriter p=new PrintWriter(Files.newBufferedWriter(out.resolve(entry+".refs")))) {
     var it=currentProgram.getListing().getInstructions(f.getBody(),true);
     while(it.hasNext()) {var i=it.next(); for(var ref:i.getReferencesFrom()) {
      var dest=ref.getToAddress(); var data=getDataAt(dest);
      p.println(i.getAddress()+" "+ref.getReferenceType()+" "+dest+(data==null?"":" "+data.toString()));
     }}
    }
   }
  } finally {d.dispose();}
 }
}
