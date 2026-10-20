// Export explicit targets from the isolated runtime project, without autoanalysis.
// @category GearsVR
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.io.*;

public class DecompileSnapshotTargets extends GhidraScript {
    public void run() throws Exception {
        if (!currentProgram.getName().startsWith("section_"))
            throw new IllegalArgumentException("Only the isolated runtime snapshot is accepted");
        String[] args = getScriptArgs();
        if (args.length < 2 || args.length > 17)
            throw new IllegalArgumentException("Output directory followed by 1–16 absolute addresses required");
        Path out = Paths.get(args[0]); Files.createDirectories(out);
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        try (PrintWriter index = new PrintWriter(Files.newBufferedWriter(out.resolve("index.tsv")))) {
            index.println("entry\tbytes\tdecompiled\terror");
            for (int n = 1; n < args.length; ++n) {
                monitor.checkCancelled(); Address entry = toAddr(args[n]);
                var block = currentProgram.getMemory().getBlock(entry);
                if (block == null || !block.isExecute())
                    throw new IllegalArgumentException("Not executable snapshot memory: " + entry);
                Function function = getFunctionAt(entry);
                if (function == null) {
                    Function containing = getFunctionContaining(entry);
                    if (containing != null)
                        throw new IllegalArgumentException("Target is inside another function: " + entry);
                    disassemble(entry);
                    CreateFunctionCmd command = new CreateFunctionCmd(entry);
                    if (!command.applyTo(currentProgram, monitor)) {
                        index.println(entry + "\t0\tfalse\t" + command.getStatusMsg());
                        continue;
                    }
                    function = getFunctionAt(entry);
                }
                DecompileResults result = decompiler.decompileFunction(function, 30, monitor);
                index.println(entry + "\t" + function.getBody().getNumAddresses() + "\t" + result.decompileCompleted() + "\t" + result.getErrorMessage());
                if (result.decompileCompleted())
                    Files.writeString(out.resolve(entry + ".c"), result.getDecompiledFunction().getC());
                try (PrintWriter assembly = new PrintWriter(Files.newBufferedWriter(out.resolve(entry + ".asm")))) {
                    var instructions = currentProgram.getListing().getInstructions(function.getBody(), true);
                    while (instructions.hasNext()) {
                        var instruction = instructions.next();
                        assembly.println(instruction.getAddress() + " " + instruction);
                    }
                }
            }
        } finally { decompiler.dispose(); }
        println("Explicit snapshot targets exported: " + out);
    }
}
