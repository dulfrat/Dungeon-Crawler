/*
 * Proyecto: Dungeon Crawler en consola - "El Descenso al Nucleo"
 * Fecha:            xx/11/2026
 * Nombre y Apellido: Jesús Ochoa  /  Ariam Flores
 * Cedula:            31.828.310  /  3
 * Numero de Seccion: 3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENEMIGOS 100   /* capacidad maxima del bestiario */
#define MAX_OBJETOS  100   /* capacidad maxima del catalogo de objetos */
#define MAX_MOCHILA   50   /* tope fisico del arreglo de la mochila */

typedef char StringCorto[15];
typedef char StringLargo[100];

typedef struct {
    StringLargo nombreObjeto;
    int idObjeto, fuerzaValor;
    StringCorto efecto;          /* Cura_Vida, Cura_Max, Buff_Daño, Buff_Defensa y esas cosas */
} Objeto;

typedef struct {
    int idEnemigo, vidaBase, danoBase, defensaBase;
    StringLargo nombreEnemigo;
    StringCorto tipo;
} Entidad;

typedef struct {
    /* atributos */
    int nivel, hpActual, hpLimite, ataque, defensa, experiencia;
    /* entorno */
    int pisoActual, pisosTotales, cuartoActual, cuartosPorPiso;
    /* inventario: espacioInventarioActual = cantidad de objetos en la mochila */
    int espacioInventarioActual, espacioInventarioLimite;
    Objeto mochila[MAX_MOCHILA];
    /* estadisticas para el highscore */
    int enemigosDerrotados, objetosConsumidos, cuartosExplorados;
} EstadoJuego;

/* Modulo 1: Inicializacion y Archivos Input */
void cargarConfiguracion(EstadoJuego *estado);
void cargarBestiario(Entidad bestiario[], int *total_enemigos);
void cargarObjetos(Objeto catalogo[], int *total_objetos);
int leerLineaNoVacia(FILE *archivo, char destino[], int tamano);

/* Modulo 2: Logica de Exploracion y Navegacion */
void avanzarCuarto(EstadoJuego *estado, Entidad bestiario[], int total_enemigos, Objeto catalogo[], int total_objetos);
Entidad seleccionarEnemigo(Entidad bestiario[], int total_enemigos, int es_cuarto_final);
int cuartosEnPiso(EstadoJuego *estado);
void revisarProgresoPiso(EstadoJuego *estado, int totalCuartos);

/* (Los demas modulos xdxd) */

int main(){
    int total_enemigos, total_objetos;
    EstadoJuego jugador;
    Entidad bestiario[MAX_ENEMIGOS];
    Objeto catalogo[MAX_OBJETOS];

    cargarConfiguracion(&jugador);
    cargarBestiario(bestiario, &total_enemigos);
    cargarObjetos(catalogo, &total_objetos);

    if(total_enemigos == 0 || total_objetos == 0){
        printf("No se pudo continuar: catalogos vacios.\n");
        return 1;
    }

    /* prueba: recorrer toda la mazmorra sin combate (se elimina despues) */
    while(jugador.pisoActual <= jugador.pisosTotales){
        avanzarCuarto(&jugador, bestiario, total_enemigos, catalogo, total_objetos);
    }
    return 0;
}

/* ===================================================================
 *  MODULO 1: INICIALIZACION Y ARCHIVOS INPUT
 * =================================================================== */

void cargarConfiguracion(EstadoJuego *estado){
    FILE *parametros = fopen("config.txt", "r");
    int semilla, leidos;

    if(parametros == NULL){
        printf("Error al abrir el archivo de entrada 'config.txt'\n");
        exit(1);
    }

    leidos = fscanf(parametros, "%d %d %d %d %d %d %d",
        &semilla,
        &((*estado).pisosTotales),
        &((*estado).cuartosPorPiso),
        &((*estado).hpLimite),
        &((*estado).ataque),
        &((*estado).defensa),
        &((*estado).espacioInventarioLimite));
    fclose(parametros);

    if(leidos != 7){
        printf("Error: 'config.txt' incompleto o con formato invalido\n");
        exit(1);
    }
    if((*estado).pisosTotales < 1 || (*estado).cuartosPorPiso < 2 ||
       (*estado).hpLimite < 1 || (*estado).espacioInventarioLimite < 1){
        printf("Error: valores invalidos en 'config.txt'\n");
        exit(1);
    }
    if((*estado).espacioInventarioLimite > MAX_MOCHILA){
        (*estado).espacioInventarioLimite = MAX_MOCHILA;
    }

    srand(semilla);

    /* valores de inicio de una partida nueva */
    (*estado).nivel = 1;
    (*estado).experiencia = 0;
    (*estado).hpActual = (*estado).hpLimite;
    (*estado).pisoActual = 1;
    (*estado).cuartoActual = 0;
    (*estado).espacioInventarioActual = 0;
    (*estado).enemigosDerrotados = 0;
    (*estado).objetosConsumidos = 0;
    (*estado).cuartosExplorados = 0;
}

// lee la siguiente line no vacia del archivo y le quita el salto de linea
int leerLineaNoVacia(FILE *archivo, char destino[], int tamano){
    int leida = 0;

    while(!leida && fgets(destino, tamano, archivo) != NULL){
        destino[strcspn(destino, "\r\n")] = '\0';
        if(strlen(destino) > 0){
            leida = 1;
        }
    }
    return leida;
}

void cargarBestiario(Entidad bestiario[], int *total_enemigos){
    FILE *archivo = fopen("bestiario.txt", "r");
    int i = 0;

    if(archivo == NULL){
        printf("Error al abrir el archivo de entrada 'bestiario.txt'\n");
        *total_enemigos = 0;
        return;
    }

    while(i < MAX_ENEMIGOS &&
          leerLineaNoVacia(archivo, bestiario[i].nombreEnemigo, sizeof(bestiario[i].nombreEnemigo)) &&
          fscanf(archivo, "%d %d %d %d %14s",
            &(bestiario[i].idEnemigo),
            &(bestiario[i].vidaBase),
            &(bestiario[i].danoBase),
            &(bestiario[i].defensaBase),
            bestiario[i].tipo) == 5){
        i++;
    }
    *total_enemigos = i;
    fclose(archivo);
}

void cargarObjetos(Objeto catalogo[], int *total_objetos){
    FILE *archivo = fopen("objetos.txt", "r");
    int i = 0;

    if(archivo == NULL){
        printf("Error al abrir el archivo de entrada 'objetos.txt'\n");
        *total_objetos = 0;
        return;
    }

    while(i < MAX_OBJETOS &&
          leerLineaNoVacia(archivo, catalogo[i].nombreObjeto,
                           sizeof(catalogo[i].nombreObjeto)) &&
          fscanf(archivo, "%d %d %14s",
            &(catalogo[i].idObjeto),
            &(catalogo[i].fuerzaValor),
            catalogo[i].efecto) == 3){
        i++;
    }
    *total_objetos = i;
    fclose(archivo);
}

/* ===================================================================
 *  MODULO 2: LOGICA DE EXPLORACION Y NAVEGACION
 * =================================================================== */

/*  piso 1: cuartosPorPiso - 1
    ultimo piso: cuartosPorPiso + 1 (cuarto extra del encuentro final)
    pisos intermedios: cuartosPorPiso
    si solo hay un piso, es a la vez el primero y el ultimo (-1 y +1) y queda con la cantidad base. */
int cuartosEnPiso(EstadoJuego *estado){
    int cuartos = (*estado).cuartosPorPiso;

    if((*estado).pisoActual == 1){
        cuartos = cuartos - 1;
    }
    if((*estado).pisoActual == (*estado).pisosTotales){
        cuartos = cuartos + 1;
    }
    return cuartos;
}

Entidad seleccionarEnemigo(Entidad bestiario[], int total_enemigos, int es_cuarto_final){
    int indicesValidos[MAX_ENEMIGOS],
    contador = 0, i;

    for(i = 0; i < total_enemigos; i++){
        if(es_cuarto_final){
            if(strcmp(bestiario[i].tipo, "Raro") == 0){
                indicesValidos[contador] = i;
                contador++;
            }
        } else {
            if(strcmp(bestiario[i].tipo, "Comun") == 0){
                indicesValidos[contador] = i;
                contador++;
            }
        }
    }

    if(contador == 0){
        return bestiario[rand() % total_enemigos];
    }
    return bestiario[indicesValidos[rand() % contador]];
}

void avanzarCuarto(EstadoJuego *estado, Entidad bestiario[], int total_enemigos, Objeto catalogo[], int total_objetos){
    int totalCuartos = cuartosEnPiso(estado),
    es_cuarto_final;
    Entidad enemigo;

    (*estado).cuartoActual++;
    (*estado).cuartosExplorados++;
    es_cuarto_final = ((*estado).cuartoActual == totalCuartos);

    printf("\n==================================================\n");
    printf(" Piso %d de %d | Cuarto %d de %d\n",
        (*estado).pisoActual, (*estado).pisosTotales,
        (*estado).cuartoActual, totalCuartos);
    printf("==================================================\n");

    enemigo = seleccionarEnemigo(bestiario, total_enemigos, es_cuarto_final);

    if(es_cuarto_final){
        printf(" Un enemigo RARO bloquea la salida del piso!\n");
    } else {
        printf(" Un enemigo ha aparecido en el camino!\n");
    }
    printf(" Enemigo: %s | Tipo: %s | Vida: %d | Dano: %d | Defensa: %d\n",
        enemigo.nombreEnemigo, enemigo.tipo, enemigo.vidaBase, enemigo.danoBase, enemigo.defensaBase);

    (void)catalogo;
    (void)total_objetos;

    revisarProgresoPiso(estado, totalCuartos);
}

void revisarProgresoPiso(EstadoJuego *estado, int totalCuartos){
    if((*estado).cuartoActual >= totalCuartos){
        printf("\n Has limpiado por completo el piso %d!\n", (*estado).pisoActual);
        (*estado).pisoActual++;
        (*estado).cuartoActual = 0;

        if((*estado).pisoActual > (*estado).pisosTotales){
            printf("\n FELICIDADES! Has superado el ultimo cuarto y conquistado el Nucleo. VICTORIA!\n");
        } else {
            printf(" Descendiendo al piso %d...\n", (*estado).pisoActual);
        }
    }
}
