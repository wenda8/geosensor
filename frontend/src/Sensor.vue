<template>
  <div id="app">
    <div class="map-container">
      <div id="map"></div>
    </div>
    <div class="chart-container">
      <div class="node">Node: 
        <span v-if="selectedNodeIds !== null">{{ selectedNodeIds }}</span>
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

        <div class="download-button-container">
          <button class="download-button" @click="downloadChartData">Download Data</button>
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

        temperatureChartData: [],
        humidityChartData: [],

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

        selectedNodeIds: [], 
        selectedNodeSensor: null,
        selectedNodeType: null,
        labels: [], // 用于存储时间标签的数组
        allData: [],
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
            if (!this.selectedNodeIds.includes(nodeId)) {
              this.selectedNodeIds.push(nodeId);
            } else {
              const index = this.selectedNodeIds.indexOf(nodeId);
              this.selectedNodeIds.splice(index, 1);
            }
            let popupContent = `Selected Node IDs: ${this.selectedNodeIds.join(', ')}<br>`;
            nodeData.forEach(node => {
              
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


      generateRandomColor() {
        const r = Math.floor(Math.random() * 256);
        const g = Math.floor(Math.random() * 256);
        const b = Math.floor(Math.random() * 256);
        return `rgb(${r}, ${g}, ${b})`;
      },
      
      // downloadChartData() {
      //   // 创建 CSV 文件的标题行
      //   let csvContent = "data:text/csv;charset=utf-8,Node ID,Type,Time,Value\n";

      //   // 遍历温度和湿度图表中的数据集，组合所有节点的数据
      //   const datasets = [...this.temperatureChartData, ...this.humidityChartData];

      //   datasets.forEach(dataset => {
      //     const labelParts = dataset.label.split(' - ');
      //     const nodeId = labelParts[0].replace('Node ', '');
      //     const type = labelParts[1];
      //     dataset.data.forEach((value, index) => {
      //       const time = this.labels[index];
      //       csvContent += `${nodeId},${type},${time},${value}\n`;
      //     });
      //   });

      //   // 创建一个虚拟链接，生成并自动触发下载
      //   const encodedUri = encodeURI(csvContent);
      //   const link = document.createElement("a");
      //   link.setAttribute("href", encodedUri);
      //   link.setAttribute("download", "chart_data.csv");
      //   document.body.appendChild(link);

      //   // 触发下载并移除虚拟链接
      //   link.click();
      //   document.body.removeChild(link);
      // },


      downloadChartData() {
        // 创建 CSV 文件的标题行
        if (!Array.isArray(this.allData) || this.allData.length === 0) {
          console.warn('No data available to export.');
          return; // 无数据时直接返回
        }
        let csvContent = "data:text/csv;charset=utf-8,node_id,sensor_id,time,env_t,env_h,soil_t,soil_h\n";

        // 遍历 `allData`，组合所有节点的完整数据
      
        this.allData.forEach(nodeDataGroup => {
          const { nodeId, nodeData } = nodeDataGroup;
          nodeData.forEach(item => {
            const timeOrDate = this.timescale === 'daily' ? item.date : item.time;
            const row = [
              nodeId,
              item.sensor_id || '', // 确保为空值时输出空字符串
              timeOrDate || '', // 使用 `date` 或 `time` 字段
              item.env_t != null ? item.env_t : '', // 确保空值时输出空字符串
              item.env_h != null ? item.env_h : '',
              item.soil_t != null ? item.soil_t : '',
              item.soil_h != null ? item.soil_h : ''
            ].join(',');

            // 添加到 CSV 内容
            csvContent += `${row}\n`;
          });
        });

        // 创建一个虚拟链接，生成并自动触发下载
        const encodedUri = encodeURI(csvContent);
        const link = document.createElement("a");
        link.setAttribute("href", encodedUri);
        link.setAttribute("download", "chart_data.csv");
        document.body.appendChild(link);

        // 触发下载并移除虚拟链接
        link.click();
        document.body.removeChild(link);
      },
      // downloadChartData() {
      //   // 检查 `allData` 是否存在且为数组
      //   if (!Array.isArray(this.allData) || this.allData.length === 0) {
      //     console.warn('No data available to export.');
      //     return;
      //   }

      //   // 创建 CSV 文件的标题行
      //   let csvContent = "data:text/csv;charset=utf-8,node_id,sensor_id,type,time,env_t,env_h,soil_t,soil_h\n";

      //   // 遍历 `allData`，组合所有节点的完整数据
      //   this.allData.forEach(nodeDataGroup => {
      //     const { nodeId, datasets } = nodeDataGroup;
  

      //     // 遍历数据集，使用 `label` 属性中的类型信息
      //     datasets.forEach(dataset => {
      //       // 从 `label` 属性中提取类型
      //       const labelParts = dataset.label.split(' - ');
      //       const type = labelParts[1]; // 获取 `type` 部分

      //       // 遍历每个数据项，将 `type` 填入对应列
      //       dataset.data.forEach((value, index) => {
      //         const time = this.labels[index];

      //         // 构造每行数据
      //         const row = [
      //           nodeId,
      //           dataset.sensor_id || '',
      //           type, // 使用 `label` 中的类型信息
      //           time,
      //           value.env_t != null ? value.env_t : '',
      //           value.env_h != null ? value.env_h : '',
      //           value.soil_t != null ? value.soil_t : '',
      //           value.soil_h != null ? value.soil_h : ''
      //         ].join(',');

      //         // 添加到 CSV 内容
      //         csvContent += `${row}\n`;
      //       });
      //     });
      //   });

      //   // 创建虚拟链接，生成并自动触发下载
      //   const encodedUri = encodeURI(csvContent);
      //   const link = document.createElement("a");
      //   link.setAttribute("href", encodedUri);
      //   link.setAttribute("download", "chart_data.csv");
      //   document.body.appendChild(link);

      //   // 触发下载并移除虚拟链接
      //   link.click();
      //   document.body.removeChild(link);
      // },
          
      async sendtime() {
        const startDateTime = `${this.selectedYearStart}-${this.selectedMonthStart.toString().padStart(2, '0')}-${this.selectedDayStart.toString().padStart(2, '0')} ${this.selectedHourStart.toString().padStart(2, '0')}`;
        const endDateTime = `${this.selectedYearEnd}-${this.selectedMonthEnd.toString().padStart(2, '0')}-${this.selectedDayEnd.toString().padStart(2, '0')} ${this.selectedHourEnd.toString().padStart(2, '0')}`;

        const allData = [];
        const datasets_temp = [];
        const datasets_humidity = [];
        this.allData = []; // 确保清空旧的数据

        // 循环发送请求，收集每个节点的数据
        for (const nodeId of this.selectedNodeIds) {
          const payload = {
            node_id: nodeId,
            time_scale: this.timescale,
            start_date: startDateTime,
            end_date: endDateTime
          };

          console.log(`Sending data for node ${nodeId}:`, payload);

          try {
            const response = await axios.get(`http://localhost:8888/period_of_time/get_period_of_time`, { params: payload });
            console.log('Data received from server:', response.data);
            const nodeData = response.data;
            if (response && response.data) {
              // 确保将 `nodeId` 和 `nodeData` 一起存储
              this.allData.push({
                nodeId,
                nodeData: response.data
              });
            }
            // 生成环境温度和土壤温度的随机颜色
            const envTColor = this.generateRandomColor();
            const soilTColor = this.generateRandomColor();
            const envHColor = this.generateRandomColor();
            const soilHColor = this.generateRandomColor();

            // 处理标签和数据
            let labels;
              if (this.timescale === 'daily') {
                labels = nodeData.map(item => item.date);
              } else if (this.timescale === 'hourly') {
                labels = nodeData.map(item => item.time);
              }
            allData.push({ nodeId, labels, nodeData });

            // 按不同节点分别添加到 datasets 中，使用不同的颜色区分
            datasets_temp.push({
              label: `Node ${nodeId} - Environment Temp`,
              data: nodeData.map(item => item.env_t),
              borderColor: envTColor,
              backgroundColor: envTColor.replace('rgb', 'rgba').replace(')', ', 0.5)')
            });

            datasets_temp.push({
              label: `Node ${nodeId} - Soil Temp`,
              data: nodeData.map(item => item.soil_t),
              borderColor: soilTColor,
              backgroundColor: soilTColor.replace('rgb', 'rgba').replace(')', ', 0.5)')
            });

            datasets_humidity.push({
              label: `Node ${nodeId} - Environment Humidity`,
              data: nodeData.map(item => item.env_h),
              borderColor: envHColor,
              backgroundColor: envHColor.replace('rgb', 'rgba').replace(')', ', 0.5)')
            });

            datasets_humidity.push({
              label: `Node ${nodeId} - Soil Humidity`,
              data: nodeData.map(item => item.soil_h),
              borderColor: soilHColor,
              backgroundColor: soilHColor.replace('rgb', 'rgba').replace(')', ', 0.5)')
            });

          } catch (error) {
            console.error(`Error fetching data for node ${nodeId}:`, error);
          }
        }

        // 获取时间轴的标签
        const commonLabels = allData.length > 0 ? allData[0].labels : [];

        // 调用图表渲染函数
        this.renderChart('temperatureChart', commonLabels, datasets_temp);
        this.renderChart('humidityChart', commonLabels, datasets_humidity);
        this.temperatureChartData = datasets_temp;
        this.humidityChartData = datasets_humidity;
        this.labels = commonLabels; // 使用最后获取的时间标签
      },

      groupDataByNode(data) {
        return data.reduce((acc, item) => {
          if (!acc[item.node_id]) {
            acc[item.node_id] = [];
          }
          acc[item.node_id].push(item);
          return acc;
        }, {});
    
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

  .send-button, .download-button {
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

.send-button, .download-button :hover {
  background-color: #1f6113; 
  box-shadow: 0 6px 12px rgba(0,0,0,0.3); 
}

.send-button, .download-button :active {
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