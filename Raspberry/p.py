import serial, time
import MySQLdb

DB_HOST = 'localhost'
DB_USER = 'root'
DB_PASS = ''
DB_NAME = 'db2'


umbral_ldr=100;
move=False;


print('Inicializando...')

micro = serial.Serial('/dev/ttyACM0',9600,timeout=1)

time.sleep(5)


def run_query(query=''):
    datos = [DB_HOST,DB_USER,DB_PASS,DB_NAME]
    conn = MySQLdb.connect(*datos)
    cursor= conn.cursor()
    cursor.execute(query)
    conn.commit()
    
    cursor.close
    conn.close()
    return
    


while(1):    
    if(micro.isOpen()):
        
        micro.write(b'[S]')
        
#         micro.write(b'[A,0,9]')

        cadena = micro.readline()
        cortada = cadena.decode("utf-8")
        if(cadena != b''):
            
            cortada= cortada.lstrip("[O")
            cortada= cortada.rstrip("]\r\n")
            separadas = cortada.split(",")
            print("Los valores de la trama son:")
            print("El LDR "+ separadas[0]+" La temperatura "+separadas[1]+" La humedad "+separadas[2]+" El checksum "+separadas[3])

            
            if (int(separadas[0])) > umbral_ldr:
                print("Se ha superado el umbral de luz, moviendo el servo")
                micro.write(b'[A,0,9]')
                move=True;

              
            else:
                micro.write(b'[A,0,1]')
                move=False;
                
            if(int(separadas[3]) == 1):
                print("Checksum superado")
                query="INSERT INTO tabla2 (ldr,temperatura,humedad) VALUES ('%s'" % separadas[0]
                query += ",'%s'" % separadas[1]
                query += ",'%s')" % separadas[2]
                
                run_query(query)
        
        
    time.sleep(5)
    #micro.write(b'[A,0,9]')

    if(move is True):
        #print(move)
        micro.write(b'[A,0,9]')
        #move=False;
    else:
        #print(move)
        micro.write(b'[A,0,0]')
        


        
    
micro.close()





