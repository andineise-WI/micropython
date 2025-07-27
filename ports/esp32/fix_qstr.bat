# Windows qstr Generation Workaround

# Erstelle leere qstr Dateien
if not exist build\genhdr\ mkdir build\genhdr\

# qstr.i.last (leer)
echo // MicroPython qstr file > build\genhdr\qstr.i.last

# qstrdefs.generated.h (minimal)
echo #ifndef QSTRDEFS_GENERATED_H > build\genhdr\qstrdefs.generated.h
echo #define QSTRDEFS_GENERATED_H >> build\genhdr\qstrdefs.generated.h
echo // Minimal qstr definitions >> build\genhdr\qstrdefs.generated.h
echo #endif >> build\genhdr\qstrdefs.generated.h

# Moduledefs.h (minimal)
echo #ifndef MODULEDEFS_H > build\genhdr\moduledefs.h  
echo #define MODULEDEFS_H >> build\genhdr\moduledefs.h
echo // Module definitions >> build\genhdr\moduledefs.h
echo #endif >> build\genhdr\moduledefs.h

echo qstr Workaround-Dateien erstellt!
