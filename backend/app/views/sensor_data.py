from flask import Blueprint, jsonify
from app.models import SensorData 
from flasgger import swag_from

sensor_data_bp = Blueprint('sensordata', __name__)

@sensor_data_bp.route('/sensor_datas', methods=['GET'])
@swag_from('get_sensor_datas.yml')
def get_sensor_datas():
    sensor_datas = SensorData.query.all()
    return jsonify([sensor_data.to_dict() for sensor_data in sensor_datas])

@sensor_data_bp.route('/sensor_data/<int:sensor_id>', methods=['GET'])
@swag_from('get_sensor_data.yml')
def get_sensor_data(sensor_id):
    sensor_datas = SensorData.query.filter_by(sensor_id=sensor_id).all()
    if sensor_datas:
        return jsonify([sensor_data.to_dict() for sensor_data in sensor_datas])
    else:
        return jsonify({'error': 'Sensor not found'}), 404
