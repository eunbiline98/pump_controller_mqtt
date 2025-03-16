console.log("MQTT.js:", typeof mqtt !== "undefined" ? "Loaded" : "Not Loaded");

// MQTT connection options
const mqttOptions = {
  keepalive: 30,
  clientId: "Dashboard_Client_" + Math.random().toString(16).slice(2, 10),
  username: "busol-labs",
  password: "$ut0h0m312",
  protocolVersion: 4,
  clean: true,
  reconnectPeriod: 1000,
  connectTimeout: 30 * 1000,
};

// MQTT broker host and topic
const mqttHost = "ws://docker-busol.paditech.id:8083/mqtt";
const topic = "relay/state";

Swal.fire({
  title: "Menghubungkan ke MQTT...",
  text: "Mohon tunggu...",
  allowOutsideClick: false,
  allowEscapeKey: false,
  didOpen: () => {
    Swal.showLoading();
  },
});

console.log("Menghubungkan ke Broker MQTT");
const client = mqtt.connect(mqttHost, mqttOptions);

// MQTT connection success
client.on("connect", function () {
  console.log("Connected to MQTT broker");
  document.getElementById("mqttStatus").innerText = "Connected";
  Swal.close();

  client.subscribe(topic, { qos: 1 }, function (err) {
    if (err) {
      document.getElementById("mqttStatus").innerText = "Disconnected";
    }
  });
});

client.on("reconnect", () => {
  console.log("MQTT Mencoba Menghubungkan Ulang...");
  document.getElementById("mqttStatus").innerText = "Reconnecting...";
  Swal.fire({
    title: "Mencoba Koneksi Ulang...",
    text: "Mohon tunggu...",
    icon: "info",
    allowOutsideClick: false,
    showConfirmButton: false,
  });
});

client.on("error", function (error) {
  console.error("MQTT connection error:", error);
  Swal.fire({
    title: "Gagal Terhubung!",
    text: "Periksa koneksi MQTT dan coba lagi.",
    icon: "error",
    confirmButtonText: "OK",
  });
});

client.on("close", function () {
  console.log("MQTT disconnected!");
  document.getElementById("mqttStatus").innerText = "Disconnected";
  disconnectMqttSwal();
});

client.on("message", function (topic, message) {
  const messageString = message.toString();
  console.log(`Message received on topic ${topic}:`, messageString);

  if (topic === "relay/state") {
    document.getElementById("relayStatus").innerText = messageString;
  }
});

document.addEventListener("DOMContentLoaded", function () {
  const relayStatus = document.getElementById("relayStatus");
  const mqttStatus = document.getElementById("mqttStatus");
  const btnOn = document.getElementById("btnOn");
  const btnOff = document.getElementById("btnOff");

  // Simulasi perubahan status relay
  btnOn.addEventListener("click", function () {
    relayStatus.style.color = "#00c853";
    Swal.fire("Relay Nyala!", "Relay telah diaktifkan.", "success");
  });

  btnOff.addEventListener("click", function () {
    relayStatus.style.color = "#d32f2f";
    Swal.fire("Relay Mati!", "Relay telah dimatikan.", "error");
  });
});

// Event listener untuk tombol
const btnOn = document.getElementById("btnOn");
const btnOff = document.getElementById("btnOff");
const btnSettings = document.createElement("button");
btnSettings.innerText = "Setting";
btnSettings.classList.add("button", "settings");
document.querySelector(".container").appendChild(btnSettings);

if (btnOn && btnOff) {
  btnOn.addEventListener("click", () => {
    if (client.connected) {
      client.publish("relay/cmnd", "ON");
      client.publish("relay/state", "ON", { retain: true });
    } else {
      disconnectMqttSwal();
    }
  });

  btnOff.addEventListener("click", () => {
    if (client.connected) {
      client.publish("relay/cmnd", "OFF");
      client.publish("relay/state", "OFF", { retain: true });
    } else {
      disconnectMqttSwal();
    }
  });
}

btnSettings.addEventListener("click", () => {
  let lastMinPressure = 10;
  let lastMaxPressure = 90;

  // Subscribe ke MQTT untuk mendapatkan last state
  client.subscribe("set/pressure", { qos: 0 });

  client.on("message", (topic, message) => {
    if (topic === "set/pressure") {
      try {
        const payload = JSON.parse(message.toString());
        lastMinPressure = parseInt(payload.down);
        lastMaxPressure = parseInt(payload.up);
      } catch (error) {
        console.error("Gagal parsing JSON dari MQTT:", error);
      }
    }
  });

  // Tunggu 1 detik sebelum menampilkan Swal
  setTimeout(() => {
    Swal.fire({
      title: "Pengaturan Pressure",
      html: `
        <label for='pressureMinSlider'>Batas Bawah:</label>
        <input type='range' id='pressureMinSlider' min='0' max='100' step='1' value='${lastMinPressure}'/>
        <p>Batas Bawah: <span id='pressureMinValue'>${lastMinPressure}</span></p>
        
        <label for='pressureMaxSlider'>Batas Atas:</label>
        <input type='range' id='pressureMaxSlider' min='0' max='100' step='1' value='${lastMaxPressure}'/>
        <p>Batas Atas: <span id='pressureMaxValue'>${lastMaxPressure}</span></p>
      `,
      showCancelButton: true,
      confirmButtonText: "Simpan",
      didOpen: () => {
        const minSlider = document.getElementById("pressureMinSlider");
        const maxSlider = document.getElementById("pressureMaxSlider");
        const minValue = document.getElementById("pressureMinValue");
        const maxValue = document.getElementById("pressureMaxValue");

        // Update value saat slider digerakkan
        minSlider.addEventListener("input", () => {
          let minVal = parseInt(minSlider.value);
          let maxVal = parseInt(maxSlider.value);

          // Jika batas bawah lebih besar dari batas atas, update maxSlider
          if (minVal > maxVal) {
            maxSlider.value = minVal;
            maxValue.textContent = minVal;
          }

          minValue.textContent = minVal;
        });

        maxSlider.addEventListener("input", () => {
          let minVal = parseInt(minSlider.value);
          let maxVal = parseInt(maxSlider.value);

          // Jika batas atas lebih kecil dari batas bawah, update minSlider
          if (maxVal < minVal) {
            minSlider.value = maxVal;
            minValue.textContent = maxVal;
          }

          maxValue.textContent = maxVal;
        });
      },
      preConfirm: () => {
        return new Promise((resolve) => {
          const pressureMin =
            document.getElementById("pressureMinSlider").value;
          const pressureMax =
            document.getElementById("pressureMaxSlider").value;

          const payload = JSON.stringify({
            down: pressureMin,
            up: pressureMax,
          });

          if (client && client.connected) {
            client.publish("set/pressure", payload, { retain: true });

            console.log(`Published set/pressure: ${payload}`, { retain: true });

            resolve(true);
          } else {
            resolve(false);
          }
        });
      },
    }).then((result) => {
      if (result.isConfirmed) {
        if (result.value) {
          Swal.fire({
            title: "Berhasil!",
            text: "Pengaturan Pressure berhasil disimpan di MQTT.",
            icon: "success",
            confirmButtonText: "OK",
          });
        } else {
          Swal.fire({
            title: "Gagal!",
            text: "Tidak dapat menyimpan pengaturan. Periksa koneksi MQTT.",
            icon: "error",
            confirmButtonText: "OK",
          });
        }
      }
    });
  }, 1000);
});

// Ambil elemen input timer
const timerStartInput = document.getElementById("timerStart");
const timerEndInput = document.getElementById("timerEnd");

// Muat nilai dari localStorage jika ada
window.onload = () => {
  if (localStorage.getItem("timerStart")) {
    timerStartInput.value = localStorage.getItem("timerStart");
  }
  if (localStorage.getItem("timerEnd")) {
    timerEndInput.value = localStorage.getItem("timerEnd");
  }
};

// Tambahkan tombol OK
const btnOk = document.createElement("button");
btnOk.innerText = "OK";
btnOk.classList.add("button", "ok");
document.querySelector(".timer-container").appendChild(btnOk);

// Fungsi untuk publish timer ke MQTT
function publishTimer() {
  const startValue = timerStartInput.value;
  const endValue = timerEndInput.value;

  if (startValue && endValue) {
    if (client.connected) {
      // Publish dengan retain flag agar nilai tetap tersimpan di broker
      client.publish("timer/start", startValue, { retain: true });
      client.publish("timer/end", endValue, { retain: true });

      console.log(`Published timer/start: ${startValue}`);
      console.log(`Published timer/end: ${endValue}`);

      // Simpan ke localStorage juga
      localStorage.setItem("timerStart", startValue);
      localStorage.setItem("timerEnd", endValue);

      Swal.fire({
        title: "Berhasil!",
        text: "Pengaturan timer tersimpan di MQTT dan berjalan di latar belakang",
        icon: "success",
        confirmButtonText: "OK",
      });
    } else {
      disconnectMqttSwal();
    }
  } else {
    Swal.fire({
      title: "Gagal!",
      text: "Harap isi kedua waktu sebelum menyimpan.",
      icon: "warning",
      confirmButtonText: "OK",
    });
  }
}

// Event listener untuk tombol OK
btnOk.addEventListener("click", publishTimer);

// Fungsi untuk menampilkan pop-up error MQTT disconnect
function disconnectMqttSwal() {
  Swal.fire({
    title: "Koneksi Terputus!",
    text: "MQTT tidak terhubung. Memeriksa koneksi...",
    icon: "error",
    confirmButtonText: "OK",
    timer: 5000,
    timerProgressBar: true,
  });
}
