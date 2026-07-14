#!/bin/bash
set -e

echo "=== 1. Instalando Mosquitto y Avahi ==="
sudo dnf install -y mosquitto avahi avahi-tools

echo "=== 2. Creando archivo de contraseñas ==="
sudo mosquitto_passwd -b -c /etc/mosquitto/passwd Ruiz26 RelaxedChar206
sudo chmod 600 /etc/mosquitto/passwd

echo "=== 3. Configurando Mosquitto con autenticación ==="
sudo tee /etc/mosquitto/mosquitto.conf > /dev/null <<'EOF'
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd
EOF

echo "=== 4. Habilitando e iniciando servicios ==="
sudo systemctl enable --now mosquitto avahi-daemon

echo "=== 5. Abriendo puerto 1883 en firewall ==="
sudo firewall-cmd --permanent --add-port=1883/tcp
sudo firewall-cmd --reload

echo "=== 6. Agregando usuario al grupo dialout (para Arduino) ==="
sudo usermod -aG dialout $USER

echo "=== 7. Verificando servicios ==="
sudo systemctl status mosquitto --no-pager
sudo systemctl status avahi-daemon --no-pager

echo ""
echo "========== LISTO =========="
echo "Hostname: $(hostname)"
echo "Broker MQTT disponible en: $(hostname).local:1883"
echo ""
echo "IMPORTANTE: Cierra sesion y vuelve a entrar"
echo "para que el grupo dialout tenga efecto."
echo "============================"
