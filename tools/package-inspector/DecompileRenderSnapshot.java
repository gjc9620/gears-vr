// Targeted analysis of a separate runtime snapshot; never run on the original PE.
// @category GearsVR
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import java.nio.file.*;
import java.io.*;
import java.util.*;

public class DecompileRenderSnapshot extends GhidraScript {
    private Function define(Address entry) throws Exception {
        Function f = getFunctionAt(entry);
        if (f != null) return f;
        disassemble(entry);
        CreateFunctionCmd cmd = new CreateFunctionCmd(entry);
        if (!cmd.applyTo(currentProgram, monitor)) {
            println("Cannot define " + entry + ": " + cmd.getStatusMsg());
            return null;
        }
        return getFunctionAt(entry);
    }
    public void run() throws Exception {
        if (!currentProgram.getName().startsWith("section_"))
            throw new IllegalArgumentException("Only an isolated section_ snapshot is accepted");
        String[] args = getScriptArgs();
        Path folder = Paths.get(args[0]);
        Path out = folder.resolve("decompiled"); Files.createDirectories(out);
        for (String line : Files.readAllLines(folder.resolve("sections.tsv"))) {
            String[] parts = line.split("\t"); Address address = toAddr(parts[1]);
            MemoryBlock block = currentProgram.getMemory().getBlock(address);
            if (block == null) {
                Path file = folder.resolve(parts[0]);
                try (InputStream stream = Files.newInputStream(file)) {
                    block = currentProgram.getMemory().createInitializedBlock(
                        parts[0], address, stream, Files.size(file), monitor, false);
                }
            }
            block.setRead(true); block.setWrite(false); block.setExecute(parts[2].equals("1"));
        }
        LinkedHashSet<Address> entries = new LinkedHashSet<>();
        for (String entry : Files.readAllLines(folder.resolve("entries.txt"))) entries.add(toAddr(entry));
        for (Address entry : entries) define(entry);
        // One bounded layer of direct callees of the scene-render candidate.
        Address first = entries.iterator().next(); // first manifest entry is RVA 15cf08
        Function scene = getFunctionAt(first.subtract(0x15cf08).add(0x11908c));
        if (scene != null) {
            InstructionIterator instructions = currentProgram.getListing().getInstructions(scene.getBody(), true);
            while (instructions.hasNext()) {
                Instruction ins = instructions.next();
                for (Reference ref : ins.getReferencesFrom()) {
                    if (!ref.getReferenceType().isCall()) continue;
                    Address target = ref.getToAddress();
                    MemoryBlock block = currentProgram.getMemory().getBlock(target);
                    if (block != null && block.isExecute() && entries.size() < 80) entries.add(target);
                }
            }
        }
        for (Address entry : entries) define(entry);
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        try (PrintWriter index = new PrintWriter(Files.newBufferedWriter(out.resolve("index.tsv")))) {
            index.println("entry\tbytes\tdecompiled\terror");
            for (Address entry : entries) {
                monitor.checkCancelled(); Function f = getFunctionAt(entry);
                if (f == null) { index.println(entry + "\t0\tfalse\tno function"); continue; }
                DecompileResults result = decompiler.decompileFunction(f, 20, monitor);
                index.println(entry + "\t" + f.getBody().getNumAddresses() + "\t" + result.decompileCompleted() + "\t" + result.getErrorMessage());
                if (result.decompileCompleted()) Files.writeString(out.resolve(entry + ".c"), result.getDecompiledFunction().getC());
                try (PrintWriter asm = new PrintWriter(Files.newBufferedWriter(out.resolve(entry + ".asm")))) {
                    InstructionIterator instructions = currentProgram.getListing().getInstructions(f.getBody(), true);
                    while (instructions.hasNext()) {
                        Instruction ins = instructions.next(); asm.println(ins.getAddress() + " " + ins);
                    }
                }
            }
        } finally { decompiler.dispose(); }
        println("Targeted snapshot analysis saved: " + out);
    }
}
