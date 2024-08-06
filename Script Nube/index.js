const mariadb = require('mariadb');
const request = require('request');

const API_KEY = ''; // ThingSpeak 'write' API key 

// Configuración de la base de datos
const dbConfig = {
  host: '127.0.0.1',
  user: 'root',
  password: '',
  database: 'db2'
};

// Crear una pool de conexiones
const pool = mariadb.createPool(dbConfig);

// Función para recuperar el dato con la ID más pequeña y cargarlo en ThingSpeak
function retrieveDataAndUpload() {
  pool.getConnection()
    .then(conn => {
      // Consulta el dato con la ID más pequeña de la base de datos MariaDB
      const query = 'SELECT ID, ldr, temperatura, humedad FROM tabla2 ORDER BY ID ASC LIMIT 1';

      conn.query(query)
        .then(rows => {
          if (rows.length > 0) {
            const data = rows[0];
            // Subir el dato a ThingSpeak
            uploadData(data);
            // Borrar el dato de la base de datos
            deleteData(data.ID);
          } else {
            console.log('No se encontraron datos en la base de datos.');
          }
          // Devuelve la conexión al pool
          conn.release();
        })
        .catch(err => {
          console.error('Error al consultar datos en la base de datos: ' + err.message);
          // Devuelve la conexión al pool incluso si hay un error
          conn.release();
        });
    })
    .catch(err => {
      console.error('Error al obtener una conexión de la piscina: ' + err);
    });
}

// Función para cargar un dato en ThingSpeak
function uploadData(data) {
  const options = {
    method: 'POST',
    url: 'https://api.thingspeak.com/update',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded'
    },
    form: {
      api_key: API_KEY,
      field1: data.ldr,
      field2: data.temperatura,
      field3: data.humedad
    }
    };

    // Mostrar los datos y la ID antes de subirlos
    console.log(`Subiendo dato - ID: ${data.ID}, ldr: ${data.ldr}, temperatura: ${data.temperatura}, humedad: ${data.humedad}`);

    request(options, function (error, response, body) {
      if (error) {
        console.log(error);
      } else {
        console.log('Dato subido exitosamente a ThingSpeak.');
      }
    });
}

// Función para borrar un dato de la base de datos
function deleteData(ID) {
  pool.getConnection()
    .then(conn => {
      const query = `DELETE FROM tabla2 WHERE ID = ${ID}`;

      conn.query(query)
        .then(() => {
          console.log(`Dato con ID ${ID} borrado de la base de datos.`);
          conn.release();
        })
        .catch(err => {
          console.error('Error al borrar dato de la base de datos: ' + err.message);
          conn.release();
        });
    })
    .catch(err => {
      console.error('Error al obtener una conexión de la piscina: ' + err);
    });
}

// Realizar la lectura y carga de datos cada 30 segundos
setInterval(retrieveDataAndUpload, 30000);
