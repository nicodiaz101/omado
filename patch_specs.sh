#!/bin/bash
awk '
/Tres iconos a la derecha/ {
    print "Cuatro iconos a la derecha (ver §3.4)."
    next
}
/### 3.4 Iconos de la Input Bar/ {
    print "### 3.4 Iconos de la Input Bar (Zona C)"
    print ""
    print "Los cuatro iconos a la derecha de la barra de entrada sirven para configurar la tarea **antes** de confirmarla, tal cual MS To Do, sumando la ventaja de las subtareas rápidas:"
    print ""
    print "| Icono | Propósito | Acción al hacer clic |"
    print "|---|---|---|"
    print "| 📅 | Fecha Límite (`due_date`) | Abre un popup con calendario para elegir hoy, mañana, u otra fecha. |"
    print "| ⏰ | Recordatorio (`reminder_at`) | Abre un popup con horas rápidas (ej. Más tarde, Mañana 9:00). |"
    print "| 🔄 | Repetición (`recurrence`) | Abre popup con opciones: Diario, Semanal, Mensual, etc. |"
    print "| 📝 | Subtareas (`steps`) | Abre un panel/popup inline que permite tipear múltiples subtareas rápidamente antes de guardar. |"
    print ""
    print "Los iconos se implementan con caracteres Unicode o fuentes de iconos (ej. Material Design Icons si se integran via font), consistente con Omarchy."
    skip = 1
    next
}
skip && /### 3.5 Proporciones/ { skip = 0 }
!skip { print }
' /home/nicolas/github/omado/SPECS.md > /home/nicolas/github/omado/SPECS_tmp.md
mv /home/nicolas/github/omado/SPECS_tmp.md /home/nicolas/github/omado/SPECS.md
