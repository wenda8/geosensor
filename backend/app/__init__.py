from flask import Flask
from flask_sqlalchemy import SQLAlchemy
from flasgger import Swagger
from flask_cors import CORS

db = SQLAlchemy()


def create_app():
    app = Flask(__name__)
    app.config['SQLALCHEMY_DATABASE_URI'] = 'postgresql://ywd:YWDa351656@rm-cn-jeo3nfa850005obo.rwlb.zhangbei.rds.aliyuncs.com:5001/Farmland data'
    db.init_app(app)
    Swagger(app)
    CORS(app)  

    from app.views.sensor_data import sensor_data_bp
    app.register_blueprint(sensor_data_bp, url_prefix='/sensor_data')
    from app.views.node_sensor import node_sensor_bp
    app.register_blueprint(node_sensor_bp, url_prefix='/node_sensor')
    from app.views.period_of_time import period_of_time_bp
    app.register_blueprint(period_of_time_bp, url_prefix='/period_of_time')

    return app
