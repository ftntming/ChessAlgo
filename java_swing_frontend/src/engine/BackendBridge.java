package engine;

import java.io.File;

public class BackendBridge {
    // Tracks whether the native library was successfully loaded
    private static boolean libraryLoaded = false;
    // Stores the error message if loading failed
    private static String libraryLoadError = null;

    static {
        final String libName = "ChessEngine";
        final String mappedName = System.mapLibraryName(libName);
        String envPath = System.getenv("CHESS_ENGINE_LIB_PATH");

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
