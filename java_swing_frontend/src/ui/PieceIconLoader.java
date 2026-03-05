package ui;

import java.util.HashMap;
import java.util.Map;

import javax.swing.ImageIcon;

public class PieceIconLoader {

    private static final Map<String, ImageIcon> cache = new HashMap<>();

    public static ImageIcon getIcon(String code) {
        if (cache.containsKey(code)) {
            return cache.get(code);
        }

        String fileName =
                (Character.isUpperCase(code.charAt(0)) ? "w" : "b")
                + Character.toLowerCase(code.charAt(0))
                + ".png";

        ImageIcon icon = null;
        // First, try loading from the classpath: /resources/<fileName>
        java.net.URL url = PieceIconLoader.class.getResource("/resources/" + fileName);
        if (url != null) {
            icon = new ImageIcon(url);
        } else {
            // Fallbacks for common development layouts (file system paths)
            String[] candidates = new String[] {
                    "resources/" + fileName,
                    "java_swing_frontend/resources/" + fileName,
                    "./resources/" + fileName
            };
            for (String p : candidates) {
                java.io.File f = new java.io.File(p);
                if (f.isFile()) {
                    icon = new ImageIcon(f.getAbsolutePath());
                    break;
                }
            }

            // As a last-ditch fallback, try classloader without leading slash
            if (icon == null) {
                url = PieceIconLoader.class.getResource("resources/" + fileName);
                if (url != null) {
                    icon = new ImageIcon(url);
                }
            }
        }

        if (icon == null) {
            // If still not found, create an empty icon to avoid NPEs and log the missing file.
            System.err.println("[PieceIconLoader] Missing piece icon: " + fileName);
            icon = new ImageIcon();
        }

        cache.put(code, icon);
        return icon;
    }
}