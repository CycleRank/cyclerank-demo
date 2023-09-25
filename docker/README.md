# Cyclerank Docker containerization

## Prerequisites
Installation of Docker is required.

## Building the images and running the containers
```
cd path/to/cyclerank-demo/
docker-compose -f docker-compose.production.yml --env-file docker/.env.production -p cyclerank-production up
```