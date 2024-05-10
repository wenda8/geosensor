from datetime import datetime
from . import db

class SensorData(db.Model):
    __tablename__ = 'sensor_data'
    sensor_id = db.Column(db.Integer, db.ForeignKey('node_sensor.sensor_id'), nullable=False, primary_key=True)
    measure_time = db.Column(db.DateTime, nullable=False, primary_key=True)
    env_t = db.Column(db.Float, nullable=True)
    env_h = db.Column(db.Float, nullable=True)
    soil_t = db.Column(db.Float, nullable=True)
    soil_h = db.Column(db.Float, nullable=True)

    node_sensor= db.relationship('NodeSensor', backref='sensor_data')

    def to_dict(self):
        return {
            'sensor_id': self.sensor_id,
            'measure_time': self.measure_time.isoformat(),
            'env_t': self.env_t,
            'env_h': self.env_h,
            'soil_t': self.soil_t,
            'soil_h': self.soil_h,
        }
    
class NodeSensor(db.Model):
    __tablename__ = 'node_sensor'
    node_id = db.Column(db.Integer, nullable=False)
    sensor_id = db.Column(db.Integer,primary_key=True)
    type = db.Column(db.String(255), nullable=False)
    latitude = db.Column(db.Float, nullable=False)
    longitude = db.Column(db.Float, nullable=False)
    
    def to_dict(self):
        return {
            'node_id': self.node_id,
            'sensor_id': self.sensor_id,
            'type': self.type,
            'latitude': self.latitude,
            'longitude': self.longitude,
        }  