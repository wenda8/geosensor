from flask import Blueprint, jsonify, request
from datetime import datetime, timedelta, timezone
from app.models import db, NodeSensor, SensorData
import numpy as np
import pytz
from sqlalchemy import func
from flasgger import swag_from
from collections import defaultdict

period_of_time_bp = Blueprint('period_of_time', __name__)


def parse_time_with_timezone(time_str, timezone='Asia/Shanghai'): 
    if not time_str:
        return None
    local = pytz.timezone(timezone)
    naive = datetime.strptime(time_str, "%Y-%m-%d %H")
    local_dt = local.localize(naive, is_dst=None)
    return local_dt

@period_of_time_bp.route('/get_period_of_time', methods=['GET'])
@swag_from('get_period_of_time.yml')
def get_period_of_time():
    node_id = request.args.get('node_id')
    time_scale = request.args.get('time_scale', 'hourly')
    start_date_str = request.args.get('start_date', '')
    end_date_str = request.args.get('end_date', '')
    sensor_type = request.args.get('type', '')  
    

    timezone = 'Asia/Shanghai' 
    start_date = parse_time_with_timezone(start_date_str, 'Asia/Shanghai') if start_date_str else datetime.now(pytz.timezone(timezone)) - timedelta(days=1)
    end_date = parse_time_with_timezone(end_date_str, 'Asia/Shanghai') if end_date_str else datetime.now(pytz.timezone(timezone))
    if not start_date or not end_date or start_date > end_date:
        return jsonify({'error': 'Invalid date range'}), 400


    if not NodeSensor.query.filter_by(node_id=node_id).first():
        return jsonify({'error': 'Node not found'}), 404
    
    sensor_ids = [record.sensor_id for record in NodeSensor.query.filter_by(node_id=node_id).all()]

    data = []
    for sensor_id in sensor_ids:
        records = SensorData.query.filter(
            SensorData.sensor_id == sensor_id,
            SensorData.measure_time >= start_date,
            SensorData.measure_time <= end_date
        ).order_by(SensorData.measure_time.asc()).all()
        
        if time_scale == 'hourly':
            data.extend([{
                'time': record.measure_time.isoformat(),
                'sensor_id': sensor_id,
                'env_t': record.env_t if 'env_t' in sensor_type or sensor_type == '' else None,
                'env_h': record.env_h if 'env_h' in sensor_type or sensor_type == '' else None,
                'soil_t': record.soil_t if 'soil_t' in sensor_type or sensor_type == '' else None,
                'soil_h': record.soil_h if 'soil_h' in sensor_type or sensor_type == '' else None
            } for record in records if record.measure_time])
        elif time_scale in ['daily', 'weekly']:
            data.extend(aggregate_data(records, time_scale, sensor_id, sensor_type))
    
    return jsonify(data)


def aggregate_data(records, scale, sensor_id, sensor_type):
    params_to_include = {
        'env_t': 'env_t' in sensor_type or sensor_type == '',
        'env_h': 'env_h' in sensor_type or sensor_type == '',
        'soil_t': 'soil_t' in sensor_type or sensor_type == '',
        'soil_h': 'soil_h' in sensor_type or sensor_type == '',
    }
    grouped = defaultdict(lambda: {k: [] for k, v in params_to_include.items() if v})

    for record in records:
        date_key = record.measure_time.date()
        for param, include in params_to_include.items():
            if include and getattr(record, param) is not None:
                grouped[date_key][param].append(getattr(record, param))

    aggregated = []
    for date_key, values in grouped.items():
        aggregated_data = {'date': date_key.strftime('%Y-%m-%d'), 'sensor_id': sensor_id}
        for param, data_list in values.items():
            if data_list:
                aggregated_data[param] = np.mean(data_list)
        aggregated.append(aggregated_data)

    return aggregated

def calculate_average(date_key, values, sensor_id):
    avg_values = {
        'date': date_key.strftime('%Y-%m-%d'),  
        'sensor_id': sensor_id 
    }
    
    for param, data_list in values.items():
        if data_list:  
            avg_values[param] = np.mean(data_list)

    return avg_values