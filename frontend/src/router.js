import { createRouter, createWebHistory } from 'vue-router';
import Sensor from '@/Sensor.vue';

const routes = [
  {
    path: '/',
    name: 'Sensor',
    component: Sensor,
  },

];

const router = createRouter({
    history: createWebHistory(process.env.BASE_URL),
    routes,
  });

export default router;