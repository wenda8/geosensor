FROM node:18.18.2

# work directory
WORKDIR /app

COPY package*.json ./

RUN npm install

COPY . .

EXPOSE 8081

CMD ["npm", "run", "serve", "--", "--port", "8081"]