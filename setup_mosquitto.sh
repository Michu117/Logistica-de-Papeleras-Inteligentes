#!/bin/bash
set -e

echo "=== 1. Instalando Mosquitto y Avahi ==="
sudo dnf install -y mosquitto avahi avahi-tools

echo "=== 2. Configurando Mosquitto ==="
echo -e "\nlistener 1883\nallow_anonymous true" | sudo tee -a /etc/mosquitto/mosquitto.conf

echo "=== 3. Habilitando e iniciando servicios ==="
sudo systemctl enable --now mosquitto avahi-daemon

echo "=== 4. Abriendo puerto 1883 en firewall ==="
sudo firewall-cmd --permanent --add-port=1883/tcp
sudo firewall-cmd --reload

echo "=== 5. Agregando usuario al grupo dialout (para Arduino) ==="
sudo usermod -aG dialout $USER

echo "=== 6. Verificando servicios ==="
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
