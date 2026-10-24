import com.android.tools.smali.dexlib2.DexFileFactory;
import com.android.tools.smali.dexlib2.Opcodes;
import com.android.tools.smali.dexlib2.iface.ClassDef;
import com.android.tools.smali.dexlib2.immutable.ImmutableDexFile;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.*;
import java.util.zip.*;
import java.io.*;

/** Isolated APK candidate: retain baseline DEX classes outside three source files. */
public final class OverlayAppDex {
    static final Set<String> SOURCES = Set.of("MavkExperimental.kt", "SessionActivity.kt", "FrontEndSetup.kt");
    static final List<String> TYPES = List.of(
        "Lcom/droiddeck/launcher/session/MavkExperimental",
        "Lcom/droiddeck/launcher/session/MavkXwaylandComponent",
        "Lcom/droiddeck/launcher/SessionActivity",
        "Lcom/droiddeck/launcher/ui/FrontEndSetupKt");
    static boolean target(ClassDef c) {
        return c.getType().startsWith("Lcom/droiddeck/launcher/") &&
            ((c.getSourceFile() != null && SOURCES.contains(c.getSourceFile())) || TYPES.stream().anyMatch(p ->
                c.getType().equals(p + ";") || c.getType().startsWith(p + "$")));
    }
    static String hash(byte[] bytes) throws Exception {
        return HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(bytes));
    }
    static byte[] canonical(Collection<? extends ClassDef> classes, Path temp) throws Exception {
        DexFileFactory.writeDexFile(temp.toString(), new ImmutableDexFile(Opcodes.forApi(29), classes));
        byte[] bytes = Files.readAllBytes(temp);
        Files.delete(temp);
        return bytes;
    }
    public static void main(String[] args) throws Exception {
        if (args.length != 5) throw new IllegalArgumentException("baseline sha256 replacement.dex output.apk workdir");
        Path baseline = Path.of(args[0]), replacement = Path.of(args[2]), output = Path.of(args[3]), work = Path.of(args[4]);
        if (!hash(Files.readAllBytes(baseline)).equals(args[1])) throw new IOException("Baseline hash mismatch");
        if (Files.exists(output)) throw new IOException("Preserve existing output");
        Files.createDirectories(work);
        var newDex = DexFileFactory.loadDexFile(replacement.toFile(), Opcodes.forApi(29));
        Set<String> newTypes = new TreeSet<>(), oldTypes = new TreeSet<>(), originalTypes = new HashSet<>();
        for (ClassDef c : newDex.getClasses()) {
            if (!target(c) || !newTypes.add(c.getType())) throw new IOException("Unexpected new class " + c.getType());
        }
        if (newTypes.isEmpty()) throw new IOException("Empty replacement");
        var container = DexFileFactory.loadDexContainer(baseline.toFile(), Opcodes.forApi(29));
        Map<String, byte[]> changed = new TreeMap<>();
        int retained = 0;
        for (String name : container.getDexEntryNames()) {
            var entry = container.getEntry(name).getDexFile();
            List<ClassDef> keep = new ArrayList<>();
            boolean touched = false;
            for (ClassDef c : entry.getClasses()) {
                if (!originalTypes.add(c.getType())) throw new IOException("Duplicate baseline class");
                if (target(c)) { oldTypes.add(c.getType()); touched = true; }
                else { keep.add(c); retained++; }
            }
            if (touched) {
                Path temp = work.resolve("check.dex");
                byte[] bytes = canonical(keep, temp);
                Path verify = work.resolve("verify.dex");
                Files.write(verify, bytes);
                var reread = DexFileFactory.loadDexFile(verify.toFile(), Opcodes.forApi(29));
                if (!Arrays.equals(bytes, canonical(reread.getClasses(), temp))) throw new IOException("DEX roundtrip differs " + name);
                Files.delete(verify);
                changed.put(name, bytes);
            }
        }
        if (oldTypes.isEmpty()) throw new IOException("No target baseline classes");
        for (String name : newTypes) {
            if (originalTypes.contains(name) && !oldTypes.contains(name)) throw new IOException("Replacement collides " + name);
        }
        Set<String> removed = new TreeSet<>(oldTypes);
        removed.removeAll(newTypes);
        for (String name : removed) if (!name.contains("$")) throw new IOException("Removed top-level class " + name);
        String extra = "classes" + (container.getDexEntryNames().size() + 1) + ".dex";
        changed.put(extra, Files.readAllBytes(replacement));
        Map<String, String> unchanged = new TreeMap<>();
        try (ZipFile zip = new ZipFile(baseline.toFile()); ZipOutputStream out = new ZipOutputStream(Files.newOutputStream(output, StandardOpenOption.CREATE_NEW))) {
            var entries = zip.entries();
            Set<String> seen = new HashSet<>();
            while (entries.hasMoreElements()) {
                ZipEntry item = entries.nextElement();
                if (!seen.add(item.getName())) throw new IOException("Duplicate ZIP entry");
                String upper = item.getName().toUpperCase(Locale.ROOT);
                if (upper.equals("META-INF/MANIFEST.MF") || upper.matches("META-INF/.*\\.(SF|RSA|DSA|EC)")) continue;
                byte[] bytes = changed.get(item.getName());
                if (bytes != null) out.putNextEntry(new ZipEntry(item.getName()));
                else {
                    bytes = zip.getInputStream(item).readAllBytes();
                    unchanged.put(item.getName(), hash(bytes));
                    out.putNextEntry(new ZipEntry(item));
                }
                out.write(bytes);
                out.closeEntry();
            }
            if (seen.contains(extra)) throw new IOException("New DEX already exists");
            out.putNextEntry(new ZipEntry(extra));
            out.write(changed.get(extra));
            out.closeEntry();
        }
        Set<String> finalTypes = new HashSet<>();
        var result = DexFileFactory.loadDexContainer(output.toFile(), Opcodes.forApi(29));
        for (String name : result.getDexEntryNames()) for (ClassDef c : result.getEntry(name).getDexFile().getClasses()) {
            if (!finalTypes.add(c.getType())) throw new IOException("Duplicate result class " + c.getType());
        }
        Set<String> expected = new HashSet<>(originalTypes);
        expected.removeAll(oldTypes);
        expected.addAll(newTypes);
        if (!finalTypes.equals(expected)) throw new IOException("Final class set mismatch");
        try (ZipFile zip = new ZipFile(output.toFile())) {
            for (var entry : unchanged.entrySet()) if (!hash(zip.getInputStream(zip.getEntry(entry.getKey())).readAllBytes()).equals(entry.getValue()))
                throw new IOException("Unexpected APK change " + entry.getKey());
        }
        System.out.println("APP_DEX_OVERLAY_PASS retainedClasses=" + retained + " oldTarget=" + oldTypes.size() + " newTarget=" + newTypes.size() +
            " unchangedMembers=" + unchanged.size() + " rewrittenDex=" + (changed.size() - 1) + " removed=" + removed);
    }
}
