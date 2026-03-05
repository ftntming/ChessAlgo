package engine;

import java.io.File;
import java.nio.charset.StandardCharsets;

// Add JNA imports for performing low-level dup2/pipe/read operations to capture native stdout
import com.sun.jna.Library;
import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.Platform;
import com.sun.jna.Pointer;

public class BackendBridge {
    // Tracks whether the native library was successfully loaded
    private static boolean libraryLoaded = false;
    // Stores the error message if loading failed
    private static String libraryLoadError = null;

    // Small JNA-facing interface for libc functions we need to duplicate stdout into a pipe
    private interface CLib extends Library {
        CLib INSTANCE = Native.load(Platform.C_LIBRARY_NAME, CLib.class);
        int pipe(int[] fds);
        int dup2(int oldfd, int newfd);
        int close(int fd);
        int read(int fd, Pointer buf, int count);
    }

    static {
        final String libName = "ChessEngine";
        final String mappedName = System.mapLibraryName(libName);
        String envPath = System.getenv("CHESS_ENGINE_LIB_PATH");

        // Attempt to redirect native stdout -> Java by creating a pipe and dup2'ing stdout
        try {
            try {
                CLib c = CLib.INSTANCE;
                int[] fds = new int[2];
                if (c.pipe(fds) == 0) {
                    final int readFd = fds[0];
                    int writeFd = fds[1];

                    // Redirect stdout (fd 1) to the pipe's write end. After dup2, close the original write fd.
                    if (c.dup2(writeFd, 1) != -1) {
                        c.close(writeFd);

                        // Start a daemon thread that reads from the read end and forwards to Java System.out
                        Thread t = new Thread(() -> {
                            try {
                                Memory buf = new Memory(4096);
                                while (true) {
                                    int r = c.read(readFd, buf, 4096);
                                    if (r <= 0) break;
                                    byte[] b = buf.getByteArray(0, r);
                                    String s = new String(b, StandardCharsets.UTF_8);
                                    // Print to Java stdout so the UI or logs can see native output
                                    System.out.print(s);
                                }
                            } catch (Throwable th) {
                                System.err.println("Native stdout reader thread failed: " + th);
                            } finally {
                                try { c.close(readFd); } catch (Throwable ignore) {}
                            }
                        }, "NativeStdoutReader");
                        t.setDaemon(true);
                        t.start();
                    } else {
                        // dup2 failed; close fds
                        try { c.close(fds[0]); } catch (Throwable ignore) {}
                        try { c.close(fds[1]); } catch (Throwable ignore) {}
                    }
                }
            } catch (Throwable t) {
                // Non-fatal: if we can't redirect, continue — native stdout will go to the process stdout as usual
                System.err.println("Failed to set up native stdout capture: " + t);
            }
        } catch (Throwable outer) {
            // swallow — this redirection is optional
            System.err.println("Unexpected error during native stdout redirection: " + outer);
        }

        // We'll try the environment path first (if provided). It may be either a file path
        // pointing directly to the shared library, or a directory that contains the library.
        Throwable firstError = null;
        if (envPath != null && !envPath.isEmpty()) {
            File p = new File(envPath);
            try {
                if (p.isFile()) {
                    // env points to the actual library file
                    System.load(p.getAbsolutePath());
                    libraryLoaded = true;
                } else if (p.isDirectory()) {
                    // env points to a directory — look for the platform-mapped filename inside it
                    File candidate = new File(p, mappedName);
                    if (candidate.isFile()) {
                        System.load(candidate.getAbsolutePath());
                        libraryLoaded = true;
                    } else {
                        firstError = new UnsatisfiedLinkError("No library file '" + mappedName + "' in directory: " + envPath);
                    }
                } else {
                    firstError = new UnsatisfiedLinkError("CHESS_ENGINE_LIB_PATH does not point to a file or directory: " + envPath);
                }
            } catch (Throwable t) {
                firstError = t;
            }

            // If env-based load didn't succeed, fall back to the normal search behavior
            if (!libraryLoaded) {
                try {
                    System.loadLibrary(libName);
                    libraryLoaded = true;
                } catch (Throwable t2) {
                    // Combine messages from the env attempt and the fallback
                    if (firstError != null) {
                        libraryLoadError = firstError.toString() + " ; fallback: " + t2.toString();
                    } else {
                        libraryLoadError = t2.toString();
                    }
                    libraryLoaded = false;
                    System.err.println("Failed to load native library '" + libName + "': " + libraryLoadError);
                    System.err.println("CHESS_ENGINE_LIB_PATH=" + envPath);
                    System.err.println("java.library.path=" + System.getProperty("java.library.path"));
                    t2.printStackTrace();
                }
            }
        } else {
            // No env var — just use the normal library path lookup
            try {
                System.loadLibrary(libName); // Name of your C++ shared library
                libraryLoaded = true;
            } catch (Throwable t) {
                libraryLoadError = t.toString();
                libraryLoaded = false;
                // Print to stderr so the error is visible when running the app
                System.err.println("Failed to load native library '" + libName + "': " + libraryLoadError);
                // Helpful diagnostic: show java.library.path so caller knows where the JVM searched
                System.err.println("java.library.path=" + System.getProperty("java.library.path"));
                t.printStackTrace();
            }
        }
    }

    // Expose a simple check for callers to verify the native library is available
    public static boolean isLibraryLoaded() {
        return libraryLoaded;
    }

    // Optional: inspect the load error (null if no error)
    public static String getLibraryLoadError() {
        return libraryLoadError;
    }

    public static native boolean applyMove(String move);
    public static native String[][] getBoardState();
    public static native void loadFEN(String fen);
    public static native boolean isCheckmate();
    public static native String[] getLegalMoves();
    public static native String getSuggestedMove();
}
