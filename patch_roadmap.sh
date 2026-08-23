#!/bin/bash
awk '
/Tres iconos a la derecha alineados/ {
    print "- 🔲 Cuatro iconos a la derecha alineados con spacing 12px (para configurar la tarea antes de crearla):"
    print "  - 📅 (Fecha límite): Abre popup/calendario para setear due_date."
    print "  - ⏰ (Recordatorio): Abre popup para setear reminder_at."
    print "  - 🔄 (Repetición): Abre popup para setear recurrence (diario, semanal, etc)."
    print "  - 📝 (Subtareas): Abre un inline-list o expande el panel derecho para agregar steps a la tarea en borrador."
    skip = 1
    next
}
skip && /^- 🔲 Tecla `N`/ { skip = 0 }
!skip { print }
' /home/nicolas/github/omado/ROADMAP.md > /home/nicolas/github/omado/ROADMAP_tmp.md
mv /home/nicolas/github/omado/ROADMAP_tmp.md /home/nicolas/github/omado/ROADMAP.md
