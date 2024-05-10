from flask import Blueprint, jsonify
from app.models import db, NodeSensor
from flasgger import swag_from

node_sensor_bp = Blueprint('node_sensor', __name__)

@node_sensor_bp.route('/node_sensors', methods=['GET'])
@swag_from('get_node_sensors.yml')
def get_node_sensors():
    node_sensors = NodeSensor.query.all()
    return jsonify([node_sensor.to_dict() for node_sensor in node_sensors])

@node_sensor_bp.route('/node_sensor/<int:node_id>', methods=['GET'])
@swag_from('get_node_sensor.yml')
def get_node_sensor(node_id):
    node_sensors = NodeSensor.query.filter_by(node_id=node_id).all()
    if node_sensors:
        return jsonify([node_sensor.to_dict() for node_sensor in node_sensors])
    return jsonify({'error': 'Node not found'}), 404
