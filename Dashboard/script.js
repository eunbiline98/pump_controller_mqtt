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
const topic = "relay/cmnd";

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
  showMqttSwal();
});

client.on("message", function (topic, message) {
  const messageString = message.toString();
  console.log(`Message received on topic ${topic}:`, messageString);

  if (topic === "relay/cmnd") {
    document.getElementById("relayStatus").innerText = messageString;
  }
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
      showSuccessSwal("Relay Dinyalakan!");
    } else {
      showMqttSwal();
    }
  });

  btnOff.addEventListener("click", () => {
    if (client.connected) {
      client.publish("relay/cmnd", "OFF");
      showSuccessSwal("Relay Dimatikan!");
    } else {
      showMqttSwal();
    }
  });
}

btnSettings.addEventListener("click", () => {
  Swal.fire({
    title: "Pengaturan Pressure",
    html: `
      <label for='pressureMinSlider'>Batas Bawah:</label>
      <input type='range' id='pressureMinSlider' min='0' max='100' step='1' value='10' oninput='updateMinPressure()'/>
      <p>Batas Bawah: <span id='pressureMinValue'>10</span></p>
      
      <label for='pressureMaxSlider'>Batas Atas:</label>
      <input type='range' id='pressureMaxSlider' min='0' max='100' step='1' value='90' oninput='updateMaxPressure()'/>
      <p>Batas Atas: <span id='pressureMaxValue'>90</span></p>
    `,
    showCancelButton: true,
    confirmButtonText: "Simpan",
    didOpen: () => {
      // Menambahkan event listener untuk update teks saat slider berubah
      const minSlider = document.getElementById("pressureMinSlider");
      const maxSlider = document.getElementById("pressureMaxSlider");
      const minValue = document.getElementById("pressureMinValue");
      const maxValue = document.getElementById("pressureMaxValue");

      minSlider.addEventListener("input", () => {
        minValue.textContent = minSlider.value;
      });

      maxSlider.addEventListener("input", () => {
        maxValue.textContent = maxSlider.value;
      });
    },
    preConfirm: () => {
      return new Promise((resolve) => {
        const pressureMin = document.getElementById("pressureMinSlider").value;
        const pressureMax = document.getElementById("pressureMaxSlider").value;

        if (client && client.connected) {
          // Publish data ke MQTT
          client.publish("pressure/min", pressureMin, { retain: true });
          client.publish("pressure/max", pressureMax, { retain: true });

          console.log(`Published pressure/min: ${pressureMin}`);
          console.log(`Published pressure/max: ${pressureMax}`);

          // Jika sukses, resolve() untuk Swal sukses
          resolve(true);
        } else {
          // Jika gagal, resolve(false) untuk Swal error
          resolve(false);
        }
      });
    },
  }).then((result) => {
    if (result.isConfirmed) {
      if (result.value) {
        // Jika berhasil publish ke MQTT
        Swal.fire({
          title: "Berhasil!",
          text: "Pengaturan Pressure berhasil disimpan di MQTT.",
          icon: "success",
          confirmButtonText: "OK",
        });
      } else {
        // Jika gagal publish ke MQTT
        Swal.fire({
          title: "Gagal!",
          text: "Tidak dapat menyimpan pengaturan. Periksa koneksi MQTT.",
          icon: "error",
          confirmButtonText: "OK",
        });
      }
    }
  });
});

// Fungsi untuk menampilkan pop-up error MQTT disconnect
function showMqttSwal() {
  Swal.fire({
    title: "Koneksi Terputus!",
    text: "MQTT tidak terhubung. Memeriksa koneksi...",
    icon: "error",
    confirmButtonText: "OK",
    timer: 5000,
    timerProgressBar: true,
  });
}

// Fungsi untuk menampilkan pop-up sukses
function showSuccessSwal(message) {
  Swal.fire({
    title: "Berhasil!",
    text: message,
    icon: "success",
    confirmButtonText: "OK",
    timer: 3000,
    timerProgressBar: true,
  });
}

// Fungsi untuk memperbarui batas bawah tekanan secara independen
function updateMinPressure() {
  const minPressureSlider = document.getElementById("pressureMinSlider");
  const minPressureValue = document.getElementById("pressureMinValue");
  minPressureValue.innerText = minPressureSlider.value;

  console.log("set/pressure/down:", minPressureSlider.value); // Debugging log

  if (client.connected) {
    client.publish("set/pressure/down", minPressureSlider.value);
  } else {
    showMqttSwal();
  }
}

// Fungsi untuk memperbarui batas atas tekanan secara independen
function updateMaxPressure() {
  const maxPressureSlider = document.getElementById("pressureMaxSlider");
  const maxPressureValue = document.getElementById("pressureMaxValue");
  maxPressureValue.innerText = maxPressureSlider.value;

  console.log("set/pressure/up:", maxPressureSlider.value, { retain: true }); // Debugging log

  if (client.connected) {
    client.publish("set/pressure/up", maxPressureSlider.value, {
      retain: true,
    });
  } else {
    showMqttSwal();
  }
}

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
      showMqttSwal();
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
