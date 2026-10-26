#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char StringCorto[15];
typedef char StringLargo[100];

typedef struct {
    //atributos
    int nivel, hpActual, hpLimite, ataque, defensa,
    //entorno
    experiencia, pisoActual, pisosTotales, cuartoActual, cuartosPorPiso,
    //inventario
    espacioInventarioActual, espacioInventarioLimite, enemigosDerrotados, objetosConsumidos;
} EstadoJuego;

typedef struct{
    int idEnemigo, vidaBase, danoBase, defensaBase;
    StringLargo nombreEnemigo;
    StringCorto tipo;
}Entidad;

typedef struct {
    StringLargo nombreObjeto;
    int fuerzaValor;
    StringCorto tipo_efecto;
}Objeto;                    // asignar objetos aleatoriamente de una lista de tipo objeto catalogo
                             //calcular efecto del objeto al ejecutarse
                             //eliminarlo si se elije soltar
                             //que haya una lista tipo objeto del inventario
                             //lo mismo con bestiario: calcular oponentes, y con ello pelea en general junto con estadoJuego
                             
                                                         
// Módulo 1: Inicialización y Archivos Input
void cargarConfiguracion(EstadoJuego *estado);
void cargarBestiario(Entidad bestiario[], int *total_enemigos);
void cargarObjetos(Objeto catalogo[], int *total_objetos);
// Módulo 2: Lógica de Exploración y Navegación
void avanzarCuarto(EstadoJuego *estado, Entidad bestiario[]);
//int total_enemigos, Objeto catalogo[], int total_objetos);
//int seleccionarEnemigo(Entidad bestiario[], int total_enemigos, int es_cuarto_final);
// Módulo 3: Sistema de Combate y Progresión
//void iniciarCombate(EstadoJuego *estado, Entidad *enemigo);
//int calcularDano(int ataque_atacante, int defensa_defensor);
//void otorgarRecompensa(EstadoJuego *estado, Objeto catalogo[], 
//int total_objetos, char tipo_enemigo[]);
//void verificarSubidaNivel(EstadoJuego *estado);
// Módulo 4: Gestión de Recursos
//void usarObjeto(EstadoJuego *estado);
// Módulo 5: Persistencia y Output (Highscore)
//void guardarPartida(EstadoJuego estado);
//int cargarPartida(EstadoJuego *estado);
//void registrarHighscore(EstadoJuego estado, bool victoria);

int main(){
    int opcion, total_enemigos, total_objetos;

    EstadoJuego jugador;
    cargarConfiguracion(&jugador);

    jugador.nivel = 1;
    jugador.pisoActual = 1;
    jugador.cuartoActual = 0;
    jugador.enemigosDerrotados = 0;
    jugador.objetosConsumidos = 0;
    jugador.experiencia = 0;

    Entidad enemigos[100];
    Objeto objetos[100];
    cargarBestiario(enemigos, &total_enemigos);
    cargarObjetos(objetos, &total_objetos);
    
    FILE *registro = fopen("highscore.txt", "a"); 
    FILE *partidaGuardada = fopen("partida_guardata.txt", "r");

    // 1. Declarar la bandera del archivo guardado
    int band_partidaGuard = (partidaGuardada != NULL);

    // 2. Mostrar el menú en pantalla
    printf("************\n");
    printf("(1) empezar nueva partida\n");
    if(band_partidaGuard){
        printf("(2) cargar partida\n");
    }
    printf("(3) highscores\n");
    printf("(4) Salir\n");
    printf("************\n");

    // 3. Ciclo para leer la opcion elegida
    do{ 
        scanf("%d", &opcion);
        
        if(!band_partidaGuard && opcion == 2){ 
            opcion = 0; // Invalidamos la opcion 2 si no hay partida
        }
        
        switch (opcion){
            case 1:
                // iniciar nueva partida
                break;
            case 2:
                // cargar partida
                break;
            case 3:
                // highscores
                break;
            case 4:
                // salir
                break;
            default:
                while (getchar() != '\n');
                printf("Ingresa el numero de alguna de las opciones que se muestra en el menu\n");
        }
    } while (opcion < 1 || opcion > 4); 

    return 0;
}

void cargarConfiguracion(EstadoJuego *estado){
    FILE *parametros = fopen("config.txt", "r");
    if(parametros == NULL){
        printf("Error al abrir el archivo de entrada 'config.txt'\n");
    } else {
        int semilla;
        fscanf(parametros, "%d", &semilla);
        srand(semilla);
        fscanf(parametros, "%d", &((*estado).pisosTotales));
        fscanf(parametros, "%d", &((*estado).cuartosPorPiso));
        fscanf(parametros, "%d", &((*estado).hpLimite));
        fscanf(parametros, "%d", &((*estado).ataque));
        fscanf(parametros, "%d", &((*estado).defensa));
        fscanf(parametros, "%d", &((*estado).espacioInventarioLimite));

        //Inicializar los valores actuales basados en los límites leídos
        (*estado).hpActual = (*estado).hpLimite;
        (*estado).espacioInventarioActual = (*estado).espacioInventarioLimite;
        fclose(parametros);
    }

}
    
/*void cargarBestiario(Entidad bestiario[], int *total_enemigos){
    int i = 0;
    FILE *enemigos = fopen("bestiario.txt", "r");
    if(enemigos == NULL){
        printf("Error al abrir el archivo de entrada 'bestiario.txt'\n");
    } else {
        // Leemos el nombre usando fgets para que acepte espacios
        // Suponiendo que el archivo intercala: 1 línea de nombre, 1 línea de stats
        while(fgets(bestiario[i].nombreEnemigo, sizeof(bestiario[i].nombreEnemigo), enemigos) != NULL){
            
            // Quitamos el salto de línea '\n' al final que deja fgets si lo hubiera
            size_t len = strlen(bestiario[i].nombreEnemigo);
            if (len > 0 && bestiario[i].nombreEnemigo[len - 1] == '\n') {
                bestiario[i].nombreEnemigo[len - 1] = '\0';
            }

            // Leemos la siguiente línea con los números y el tipo
            if(fscanf(enemigos, "%d %d %d %d %s", 
                &(bestiario[i].idEnemigo), 
                &(bestiario[i].vidaBase), 
                &(bestiario[i].danoBase), 
                &(bestiario[i].defensaBase), 
                bestiario[i].tipo) == 5) {
                printf(" %s %d %d %d %d %s", bestiario[i].nombreEnemigo, (bestiario[i].idEnemigo), (bestiario[i].vidaBase), (bestiario[i].danoBase), (bestiario[i].defensaBase), bestiario[i].tipo);
                i++; // Enemigo cargado con éxito
            } else {
                break; // Si falla la lectura de stats
            }
        }
        *total_enemigos = i;
        fclose(enemigos);
    }  
}*/

void cargarBestiario(Entidad bestiario[], int *total_enemigos){
    int i = 0;
    FILE *enemigos = fopen("bestiario.txt", "r");
    if(enemigos == NULL){
        printf("Error al abrir el archivo de entrada 'bestiario.txt'\n");
        *total_enemigos = 0;
    } else {
        // fgets lee toda la línea del nombre (con espacios incluidos)
        while(fgets(bestiario[i].nombreEnemigo, sizeof(bestiario[i].nombreEnemigo), enemigos) != NULL){
            
            // quita el salto de línea o retorno de carro del final
            bestiario[i].nombreEnemigo[strcspn(bestiario[i].nombreEnemigo, "\r\n")] = '\0';

            // si hay una línea vacía por accidente la deja en visto
            if (strlen(bestiario[i].nombreEnemigo) == 0) {
                continue;
            }

            // las demas estadísticas en la línea de abajo
            if(fscanf(enemigos, "%d %d %d %d %s", 
                &bestiario[i].idEnemigo, 
                &bestiario[i].vidaBase, 
                &bestiario[i].danoBase, 
                &bestiario[i].defensaBase, 
                bestiario[i].tipo) == 5) {
                
                // printeada historica luego la quitamos
                printf("Enemigo: [%s] | ID: %d | Vida: %d | Daño: %d | Def: %d | Tipo: %s\n", 
                    bestiario[i].nombreEnemigo, 
                    bestiario[i].idEnemigo, 
                    bestiario[i].vidaBase, 
                    bestiario[i].danoBase, 
                    bestiario[i].defensaBase, 
                    bestiario[i].tipo);
                i++;
            } else {
                break; 
            }
        }
        *total_enemigos = i;
        fclose(enemigos);
    }  
}
void cargarObjetos(Objeto catalogo[], int *total_objetos){
    int i = 0;
    FILE *objetos = fopen("objetos.txt", "r");
    if(objetos == NULL){
        printf("Error al abrir el archivo de entrada 'objetos.txt'\n");
    } else {
        while(fscanf(objetos, "%s %d %s",  (catalogo[i].nombreObjeto), &(catalogo[i].fuerzaValor), (catalogo[i].tipo_efecto)) == 3 ){
        i ++;
        }
        *total_objetos = i;
        fclose(objetos);
    }  
}
void cargarBestiario(Entidad bestiario[], int *total_enemigos){
    int i = 0;
    FILE *enemigos = fopen("bestiario.txt", "r");
    if(enemigos == NULL){
        printf("Error al abrir el archivo de entrada 'bestiario.txt'\n");
        *total_enemigos = 0;
    } else {
        // fgets lee toda la línea del nombre (con espacios incluidos)
        while(fgets(bestiario[i].nombreEnemigo, sizeof(bestiario[i].nombreEnemigo), enemigos) != NULL){
            
            // quita el salto de línea o retorno de carro del final
            bestiario[i].nombreEnemigo[strcspn(bestiario[i].nombreEnemigo, "\r\n")] = '\0';

            // si hay una línea vacía por accidente la deja en visto
            if (strlen(bestiario[i].nombreEnemigo) == 0) {
                continue;
            }

            // las demas estadísticas en la línea de abajo
            if(fscanf(enemigos, "%d %d %d %d %s", &(catalogo[i].fuerzaValor),(catalogo[i].tipo_efecto)) == 2) {
                
                // printeada historica luego la quitamos
                printf("Enemigo: [%s] | ID: %d | Vida: %d | Daño: %d | Def: %d | Tipo: %s\n", 
                    bestiario[i].nombreEnemigo, 
                    bestiario[i].idEnemigo, 
                    bestiario[i].vidaBase, 
                    bestiario[i].danoBase, 
                    bestiario[i].defensaBase, 
                    bestiario[i].tipo);
                i++;
            } else {
                break; 
            }
        }
        *total_enemigos = i;
        fclose(enemigos);
    }  
}



void avanzarCuarto(EstadoJuego *estado, Entidad bestiario[]){

}