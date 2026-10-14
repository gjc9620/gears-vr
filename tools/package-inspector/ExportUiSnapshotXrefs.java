// @category GearsVR
import ghidra.app.script.GhidraScript;
import java.nio.file.*;
import java.io.*;
public class ExportUiSnapshotXrefs extends GhidraScript {
 public void run() throws Exception {
  String[] args=getScriptArgs();
  try(PrintWriter p=new PrintWriter(Files.newBufferedWriter(Paths.get(args[0])))) {
   for(int n=1;n<args.length;n++) {
    var a=toAddr(args[n]); p.println("TARGET "+a+" "+getDataAt(a));
    var vicinity=currentProgram.getListing().getInstructions(a.subtract(48),true);
    while(vicinity.hasNext()) {var i=vicinity.next(); if(i.getAddress().compareTo(a.add(80))>0) break; p.println(i.getAddress()+" "+i);}
    var it=currentProgram.getReferenceManager().getReferencesTo(a);
    while(it.hasNext()) {var r=it.next(); var f=getFunctionContaining(r.getFromAddress());
     p.println(r+" function="+(f==null?"unknown":f.getEntryPoint()));
     var ins=getInstructionAt(r.getFromAddress()); if(ins!=null) p.println(ins);
    }
   }
  }
 }
}
