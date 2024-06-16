import os
import serial
import time
import openpyxl

def validate_data(data_str):
    required_fields = {'Env_T', 'Env_H', 'Soil_T', 'Soil_H', 'Time'}
    data_parts = data_str.split(',')
    data_dict = {}
    
    for part in data_parts:
        key_value = part.split(":")
        if len(key_value) == 2:
            key = key_value[0].strip()
            value = key_value[1].strip()
            data_dict[key] = value
    
    # 检查所有必需字段是否都存在
    if required_fields.issubset(data_dict.keys()):
        return data_dict
    return None


# 打开串口
ser = serial.Serial('COM9', 9600, timeout=1)

# 输入要获取数据的节点ID
node_id = input("Enter node ID to get data: ")

# 发送GET消息
ser.write(("GET" + node_id).encode())

# 创建Excel工作簿和工作表
wb = openpyxl.Workbook()
ws = wb.active
ws.append(["Node ID", "Time", "Env Temperature", "Env Humidity", "Soil Temperature", "Soil Humidity"])  # 添加表头

# 标志变量，指示是否至少接收到一条数据
data_received = False
last_received_time = time.time()

# 循环接收并写入数据
while True:
    # 读取串口数据
    data = ser.readline().decode('utf-8').strip()
    
    # 如果数据非空，将其解析并写入Excel表格
    if data:
        print("Raw data:", data)  # 打印收到的原始数据，用于调试
        last_received_time = time.time()  # 更新最后接收到数据的时间
        data_received = True

        parsed_data = validate_data(data)
        
        row = [node_id,
               parsed_data.get('Time', ''),
               parsed_data.get('Env_T', ''),
               parsed_data.get('Env_H', ''),
               parsed_data.get('Soil_T', ''),
               parsed_data.get('Soil_H', '')]
        ws.append(row)  # 写入一行数据

        # 打印获取到的数据
        print("Received data:")
        for key, value in parsed_data.items():
            print(f"{key}: {value}")
        print()
    
    if time.time() - last_received_time > 30:
        break
    
    # 暂停一段时间，防止读取太快
    time.sleep(0.1)

   

# 确保目标文件夹存在
folder_path = "C:/Users/ywd/Desktop/论文材料/up/"  # 指定文件夹路径
if not os.path.exists(folder_path):
    os.makedirs(folder_path)

# 保存Excel文件到指定文件夹内
filename = f"{folder_path}/node_{node_id}_data.xlsx"
wb.save(filename)

# 关闭串口
ser.close()
