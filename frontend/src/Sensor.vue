<template>
  <div id="app">
    <div class="map-container">
      <div id="map"></div>
    </div>
    <div class="chart-container">
      <div class="node">Node: 
        <span v-if="selectedNodeId !== null">{{ selectedNodeId }}</span>
      </div>
      
      <div class="Tchart">
        <label for="Temperature">Temperature </label>
        <canvas id="temperatureChart"></canvas>
      </div>

      <div class="Hchart">
        <label for="Humidity">Humidity </label>
        <canvas id="humidityChart"></canvas>
      </div>

      <div class="scale">
        <label for="Timescale">Timescale: </label>
        <select v-model="timescale">
          <option value="hourly">Hourly</option>
          <option value="daily">Daily</option>
          <!-- <option value="weekly">Weekly</option> -->
        </select>
      </div>
      
      <div class="FromToSend-container">
        <div class="From-selection">
          <label for="From">From: </label>
          <label for="fromYear">Y:</label>
          <select id="fromYear" v-model="selectedYearStart">
            <option v-for="year in years" :key="`year-${year}`" :value="year">{{ year }}</option>
          </select>

          <label for="fromMonth">M:</label>
          <select id="fromMonth" v-model="selectedMonthStart" @change="updateDaysStart">
            <option v-for="month in months" :key="`month-${month}`" :value="month">{{ month }}</option>
          </select>

          <label for="fromDay">D:</label>
          <select id="fromDay" v-model="selectedDayStart">
            <option v-for="day in daysStart" :key="`start-day-${day}`" :value="day">{{ day }}</option>
          </select>

          <label for="fromHour">H:</label>
          <select id="fromHour" v-model="selectedHourStart">
            <option v-for="hour in hours" :key="`hour-${hour}`" :value="hour">{{ hour }}</option>
          </select>
        </div>
      
        <div class="To-selection">
          <label for="To">To: </label>
          <label for="toYear">Y:</label>
          <select id="toYear" v-model="selectedYearEnd">
            <option v-for="year in years" :key="`year-${year}`" :value="year">{{ year }}</option>
          </select>

          <label for="toMonth">M:</label>
          <select id="toMonth" v-model="selectedMonthEnd" @change="updateDaysEnd">
            <option v-for="month in months" :key="`month-${month}`" :value="month">{{ month }}</option>
          </select>

          <label for="toDay">D:</label>
          <select id="toDay" v-model="selectedDayEnd">
            <option v-for="day in daysEnd" :key="`end-day-${day}`" :value="day">{{ day }}</option>
          </select>

          <label for="toHour">H:</label>
          <select id="toHour" v-model="selectedHourEnd">
            <option v-for="hour in hours" :key="`hour-${hour}`" :value="hour">{{ hour }}</option>
          </select>
        </div>
      
        <div class="send-button-container">
          <button class="send-button" @click="sendtime">Send</button>
        </div>

      </div>

    </div>
  </div>
</template>
  
  <script>
  import L from 'leaflet';
  import "leaflet/dist/leaflet.css";
  import { Chart, registerables } from 'chart.js'
  import axios from 'axios';
  import 'chartjs-adapter-date-fns';
  
  export default {
    name: 'DashboardView',
    data() {
      return {
        
        nodesData: [],
        popup: null, 

        temperatureChartData: null,
        humidityChartData: null,

        timescale: 'hourly',

        selectedYearStart: '',
        selectedMonthStart: '',
        selectedDayStart: '',
        selectedHourStart: '',
        selectedYearEnd: '',
        selectedMonthEnd: '',
        selectedDayEnd: '',
        selectedHourEnd: '',

        daysStart: [],
        daysEnd: [],
        years: [],
        months: [],
        days: [],
        hours: [],

        selectedNodeId: 1, 
        selectedNodeSensor: null,
        selectedNodeType: null,
      };
      
    },
    mounted() {
      this.initMap();
      this.fetchAllNodeData();
      this.initSelections();
      this.sendtime();
      Chart.register(...registerables);
    },

    methods: {
      initMap() {
        if (document.getElementById('map')) {
          this.map = L.map('map',{ 
                      zoomAnimation: false,
                      maxZoom: 24,
                      // crs: L.CRS.EPSG3857
                  }).setView([34.643360, 112.513100], 21);
          L.tileLayer('https://server.arcgisonline.com/arcgis/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}.png', {
            maxZoom: 24,  
            maxNativeZoom: 19, 
            attribution: 'Tiles &copy; Esri &mdash; Source: Esri, i-cubed, USDA, USGS, AEX, GeoEye, Getmapping, Aerogrid, IGN, IGP, UPR-EGP, and the GIS User Community'
          }).addTo(this.map);
          const mapContainer = document.getElementById('map');
              const resizeObserver = new ResizeObserver(entries => {
                  for (let entry of entries) {
                      console.log('map changed', entry.contentRect.width, entry.contentRect.height);
                  }
              })
              resizeObserver.observe(mapContainer);
        } else {
          console.error('Map container not found');
        }
      },
  
      async fetchAllNodeData() {
        try {
          const response = await axios.get('http://localhost:8888/node_sensor/node_sensors');
          this.nodesData = response.data;
          console.log("Fetched nodes data:", this.nodesData);
          this.processAndDisplayNodes();
        } catch (error) {
          console.error('Failed to fetch node data:', error);
        }
      },

      processAndDisplayNodes() {
        if (!this.map) {
          console.error("Map instance is not available.");
          return;
        }
        const groupedNodes = this.groupNodesByNodeId(this.nodesData);
        
        Object.entries(groupedNodes).forEach(([nodeId, nodeData]) => {
          const { latitude, longitude } = nodeData[0]; 

      
          const iconHtml = `<i class="fas fa-map-marker-alt" style="color: red; font-size: 25px;"></i>`;
          const customIcon = L.divIcon({
              html: iconHtml,
              iconSize: [30, 30],
              className: 'my-custom-icon'
          });

          const marker = L.marker([latitude, longitude], { icon: customIcon }).addTo(this.map);
          marker.on('click', () => {
            let popupContent = `Node ID: ${nodeId}<br>`;
            nodeData.forEach(node => {
              this.selectedNodeId = node.node_id;
              popupContent += `Sensor ID: ${node.sensor_id}, Type: ${node.type}<br>`;
            });
            
            this.popup = L.popup()
            .setLatLng([latitude, longitude]) 
            .setContent(popupContent) 
            .openOn(this.map); 
          });
        });
      },

      groupNodesByNodeId(nodes) {
        return nodes.reduce((acc, node) => {
          if (!acc[node.node_id]) {
            acc[node.node_id] = [];
          }
          acc[node.node_id].push(node);
          return acc;
        }, {});
      },

      initSelections() {
        const today = new Date();
        const yesterday = new Date(today);
        yesterday.setDate(yesterday.getDate() - 1); 

        this.years = Array.from({ length: 5 }, (_, i) => 2020 + i);
        this.months = Array.from({ length: 12 }, (_, i) => i + 1);
        this.hours = Array.from({ length: 24 }, (_, i) => i);
        this.selectedYearStart = new Date().getFullYear();
        this.selectedMonthStart = new Date().getMonth() + 1;
        this.selectedDayStart = yesterday.getDate();
        this.updateDaysStart();
        this.selectedHourStart = new Date().getHours();
        this.selectedYearEnd = new Date().getFullYear();
        this.selectedMonthEnd = new Date().getMonth() + 1;
        this.selectedDayEnd = today.getDate();
        this.updateDaysEnd();
        this.selectedHourEnd = new Date().getHours();
      },
      
      updateDaysStart() {
        this.updateDays(true); 
      },
      updateDaysEnd() {
          this.updateDays(false); 
      },

      updateDays(isStart) {
          let monthLength;
          const year = isStart ? this.selectedYearStart : this.selectedYearEnd;
          const month = isStart ? this.selectedMonthStart : this.selectedMonthEnd;

          if ([1, 3, 5, 7, 8, 10, 12].includes(month)) {
              monthLength = 31;
          } else if ([4, 6, 9, 11].includes(month)) {
            
              monthLength = 30;
          } else if (month === 2) {
         
              if ((year % 4 === 0 && year % 100 !== 0) || year % 400 === 0) {
                  monthLength = 29; 
              } else {
                  monthLength = 28; 
              }
          }

          if (isStart) {
              this.daysStart = Array.from({ length: monthLength }, (_, i) => i + 1);
              if (this.selectedDayStart > monthLength) {
                  this.selectedDayStart = monthLength;
              }
          } else {
              this.daysEnd = Array.from({ length: monthLength }, (_, i) => i + 1);
              if (this.selectedDayEnd > monthLength) {
                  this.selectedDayEnd = monthLength;
              }
          }
      },


      async sendtime() {
        const startDateTime = `${this.selectedYearStart}-${this.selectedMonthStart.toString().padStart(2, '0')}-${this.selectedDayStart.toString().padStart(2, '0')} ${this.selectedHourStart.toString().padStart(2, '0')}`;
        const endDateTime = `${this.selectedYearEnd}-${this.selectedMonthEnd.toString().padStart(2, '0')}-${this.selectedDayEnd.toString().padStart(2, '0')} ${this.selectedHourEnd.toString().padStart(2, '0')}`;

        const payload = {
          node_id: this.selectedNodeId,
          time_scale: this.timescale,
          start_date: startDateTime,
          end_date: endDateTime
        };
        console.log('Sending data to server:', payload);
        try {
          const sendtime = await axios.get(`http://localhost:8888/period_of_time/get_period_of_time`, { params: payload });
          console.log('Data received from server:', sendtime.data);
          this.processData(sendtime)
        } catch (error) {
          console.error('Error fetching data:', error);
        }
      },
      
      processData(data) {
        let labels;
          if (this.timescale === 'daily') {
            labels = data.data.map(item => item.date);
          } else if (this.timescale === 'hourly') {
            labels = data.data.map(item => item.time);
          }
        const envTemperatureData = data.data.map(item => ({y: item.env_t, sensorId: item.sensor_id}))
        const soilTemperatureData = data.data.map(item => ({y: item.soil_t, sensorId: item.sensor_id}))
        const envHumidityData = data.data.map(item => ({y: item.env_h, sensorId: item.sensor_id}))
        const soilHumidityData = data.data.map(item => ({y: item.soil_h, sensorId: item.sensor_id}))
        
        // console.log(envTemperatureData)

        this.renderChart('temperatureChart', labels, [
          {
            label: 'Environment Temperature',
            data: envTemperatureData.map(item => item.y),
            borderColor: 'rgb(255, 99, 132)',
            backgroundColor: 'rgba(255, 99, 132, 0.5)',
            sensorIds: envTemperatureData.map(item => item.sensorId),
          },
          {
            label: 'Soil Temperature',
            data: soilTemperatureData.map(item => item.y),
            borderColor: 'rgb(54, 162, 235)',
            backgroundColor: 'rgba(54, 162, 235, 0.5)',
            sensorIds: soilTemperatureData.map(item => item.sensorId),
          }
        ])

        this.renderChart('humidityChart', labels, [
          {
            label: 'Environment Humidity',
            data: envHumidityData.map(item => item.y),
            borderColor: 'rgb(255, 206, 86)',
            backgroundColor: 'rgba(255, 206, 86, 0.5)',
            sensorIds: envHumidityData.map(item => item.sensorId),
          },
          {
            label: 'Soil Humidity',
            data: soilHumidityData.map(item => item.y),
            borderColor: 'rgb(75, 192, 192)',
            backgroundColor: 'rgba(75, 192, 192, 0.5)',
            sensorIds: soilHumidityData.map(item => item.sensorId),
          }
        ])
      },
      renderChart(chartId, labels, datasets) {
        const ctx = document.getElementById(chartId).getContext('2d');
        const existingChart = Chart.getChart(ctx);
        
        if (existingChart) {
          existingChart.destroy();
        }

        let xAxisOptions = {};
        if (this.timescale === 'hourly') {
          xAxisOptions = {
            type: 'time',
            time: {
              parser: "yyyy-MM-dd'T'HH:mm:ssssXXX", 
              tooltipFormat: 'yyyy-MM-dd-HH:mm:ss', 
              unit: 'hour',
              displayFormats: {
                hour: 'HH:mm'
              }
            },
            ticks: {
              source: 'data' 
            }
          };
        }else if (this.timescale === 'daily') {
          xAxisOptions = {
            type: 'time',
            time: {
              parser: "yyyy-MM-dd", 
              unit: 'day',
              tooltipFormat: 'yyyy-MM-dd', 
              displayFormats: {
                day: 'yyyy-MM-dd' 
              }
            },
            ticks: {
              source: 'data'
            }
          }
        }
      
        new Chart(ctx, {
          type: 'line',
          data: {
            labels,
            datasets
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
              x: xAxisOptions 
            },
            plugins: {
              tooltip: {
                callbacks: {
                  label: function(context) {
                    let label = context.dataset.label || '';
                    if (label) {
                      label += ': ';
                    }
                    if (context.parsed.y !== null) {
                      label += context.parsed.y;
                    }
                    if (context.raw.sensorId) {
                      label += ' (Sensor ID: ' + context.raw.sensorId + ')';
                    }
                    return label;
                  }
                }
              }
            }
          } 
        })
      }
    }
  }

  </script>
  
<style>
  #app {
    display: flex;
    height: 100vh; 
  }
  
  .map-container {
    width: 50vw; 
    height: 100vh; 
    overflow: hidden;
  }

  .chart-container {
    display: flex;
    flex-direction: column; 
    justify-content: space-around; 
    padding: 10px 10px 10px 10px;
    padding-top: 0; 
    overflow: auto;
    width: 50vw;
    height: 100vh;
    margin-top: 0; 
  }

  .Tchart, .Hchart {
    width: 700px; 
    height: 300px; 
    margin: 10px auto 30px; 
  }

  #map {
    height: 100%;
    width: 100%; 
  }


  .FromToSend-container {
    display: flex;
    flex-wrap: wrap; 
    justify-content: space-around; 
    align-items: center; 
  }

  .From-selection, .To-selection, .send-button-container {
    display: flex;
    justify-content: space-around; 
    align-items: center; 
  }

  .node {
  font-size: 30px; 
  font-weight: bold; 
  color: #ffffff; 
  margin-bottom: 10px; 
  font-family: Arial, sans-serif; 
  background-color: #226d15; 
  width: 100%; 
  height: 100px; 
  display: flex; 
  justify-content: center; 
  align-items: center; 
  margin: 0 auto; 
  }

  .send-button {
  padding: 10px 15px; 
  background-color: #226d15; 
  color: white; 
  border: none; 
  border-radius: 5px; 
  cursor: pointer;
  font-size: 14px; 
  font-weight: bold; 
  text-transform: uppercase; 
  transition: all 0.3s; 
}

.send-button:hover {
  background-color: #1f6113; 
  box-shadow: 0 6px 12px rgba(0,0,0,0.3); 
}

.send-button:active {
  box-shadow: 0 2px 4px rgba(0,0,0,0.2); 
  transform: translateY(2px); 
}

.scale, .From-selection, .To-selection {
  font-family: Arial, sans-serif; 
  color: #333; 
  padding: 5px; 
  margin: 10px 0; 
}

.scale label, .From-selection label, .To-selection label {
  margin-right: 5px; 
}

.scale select, .From-selection select, .To-selection select {
  padding: 5px 5px; 
  border: 2px solid #ccc; 
  border-radius: 5px;
  background-color: #f8f8f8; 
  cursor: pointer; 
  font-size: 16px; 
  color: #333; 
  outline: none; 
}

.scale select:hover, .From-selection:hover, .To-selection:hover {
  border-color: #bbb; 
}

.scale select:focus, .From-selection:focus, .To-selection:focus {
  border-color: #007bff; 
}

</style>