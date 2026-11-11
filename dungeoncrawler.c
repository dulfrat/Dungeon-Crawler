/*
    Proyecto: Dungeon Crawler en consola - "El Descenso al Nucleo"
    Fecha:            xx/11/2026
    Nombre y Apellido: Jesús Ochoa  /  Ariam Flores
    Cedula:            31.828.310  /  3
    Numero de Seccion: 3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENEMIGOS 100
#define MAX_OBJETOS  100
#define MAX_MOCHILA   50

/*balance del jueguito este*/
#define PROB_DROP_COMUN   40   /* % de que un enemigo Comun suelte un objeto */
#define XP_BASE_NIVEL     50   /* XP para pasar del nivel N al N+1 = 50 * N */
#define BONO_HP           10   /* HP maximo que se gana por nivel */
#define BONO_ATAQUE        2   /* ataque que se gana por nivel */
#define BONO_DEFENSA       1   /* defensa que se gana por nivel */
#define HUIDA_BASE        50   /* % base de huir cuando ataque enemigo = defensa jugador */
#define HUIDA_POR_PUNTO    3   /* % que suma/resta cada punto de diferencia */
#define HUIDA_MIN         10   /* % minimo de huir */
#define HUIDA_MAX         90   /* % maximo de huir */

typedef char StringCorto[15];
typedef char StringLargo[100];

typedef struct {
    StringLargo nombreObjeto;
    int idObjeto, fuerzaValor;
    StringCorto efecto;
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

/* Modulo 3: Sistema de Combate y Progresion */
void iniciarCombate(EstadoJuego *estado, Entidad *enemigo);
int calcularDano(int ataque_atacante, int defensa_defensor);
void otorgarRecompensa(EstadoJuego *estado, Objeto catalogo[], int total_objetos, char tipo_enemigo[]);
void verificarSubidaNivel(EstadoJuego *estado);
int umbralExperiencia(int nivel);
int calcularExperiencia(Entidad enemigo);
int calcularProbabilidadHuida(int ataque_enemigo, int defensa_jugador);
int intentarHuir(EstadoJuego *estado, Entidad *enemigo);
void turnoEnemigo(EstadoJuego *estado, Entidad *enemigo);

/* Utilidad de entrada por consola */
int leerOpcion(int minimo, int maximo);

/*Modulo 4: Gestion de Recursos (provisional hasta el Bloque 4) */
void usarObjeto(EstadoJuego *estado);

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

    /* prueba: avanzar cuartos hasta ganar o morir (se elimina despues) */
    while(jugador.pisoActual <= jugador.pisosTotales && jugador.hpActual > 0){
        avanzarCuarto(&jugador, bestiario, total_enemigos, catalogo, total_objetos);
    }

    if(jugador.hpActual > 0){
        printf("\n[RESULTADO] VICTORIA\n");
    } else {
        printf("\n[RESULTADO] DERROTA\n");
    }
    printf("Nivel %d | HP %d/%d | ATK %d | DEF %d | Enemigos: %d | Cuartos: %d | Mochila: %d/%d\n",
        jugador.nivel, jugador.hpActual, jugador.hpLimite, jugador.ataque,
        jugador.defensa, jugador.enemigosDerrotados, jugador.cuartosExplorados,
        jugador.espacioInventarioActual, jugador.espacioInventarioLimite);
    return 0;
}

/* ===================================================================
    MODULO 1: INICIALIZACION Y ARCHIVOS INPUT
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
    /* validaciones basicas de los parametros */
    if((*estado).pisosTotales < 1 || (*estado).cuartosPorPiso < 2 ||
       (*estado).hpLimite < 1 || (*estado).espacioInventarioLimite < 1){
        printf("Error: valores invalidos en 'config.txt'\n");
        exit(1);
    }
    if((*estado).espacioInventarioLimite > MAX_MOCHILA){
        (*estado).espacioInventarioLimite = MAX_MOCHILA;
    }

    srand(semilla);

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
 
int leerLineaNoVacia(FILE *archivo, char destino[], int tamano){
    int leida = 0;

    while(!leida && fgets(destino, tamano, archivo) != NULL){
        /* quita el salto de linea o retorno de carro del final */
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
          leerLineaNoVacia(archivo, bestiario[i].nombreEnemigo,
                           sizeof(bestiario[i].nombreEnemigo)) &&
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

/* Lee objetos.txt. Cada objeto ocupa dos lineas:
 *   linea 1: Nombre (puede tener espacios)
 *   linea 2: [ID] [Fuerza/Valor] [Tipo_Efecto]
 * La cantidad de objetos leidos se devuelve por referencia. */
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

/* elige al azar un enemigo del bestiario: raro si es el cuarto final del piso y de tipo comun en los demas cuartos.
devuelve una copia del enemigo, asi el combate puede modificar su vida sin alterar el bestiario. Si no existe ningun enemigo del tipo buscado, elige
cualquiera para que el juego no se bloquee. */
Entidad seleccionarEnemigo(Entidad bestiario[], int total_enemigos, int es_cuarto_final){
    int indicesValidos[MAX_ENEMIGOS];
    int contador = 0;
    int i;

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

/* avanza al siguiente cuarto del piso actual, genera su enemigo y lanza el combate. Segun el resultado:
   - el jugador murio: no se hace nada mas (derrota);
   - el enemigo murio: se otorgan XP y botin, se cuenta el cuarto como explorado y se revisa si el piso quedo completo;
   - el jugador huyo: el cuarto NO se supera, se retrocede el contador para que siga pendiente (asi no se puede saltar al jefe raro). */
void avanzarCuarto(EstadoJuego *estado, Entidad bestiario[], int total_enemigos,
                   Objeto catalogo[], int total_objetos){
    int totalCuartos = cuartosEnPiso(estado);
    int es_cuarto_final;
    Entidad enemigo, original;

    (*estado).cuartoActual++;
    es_cuarto_final = ((*estado).cuartoActual == totalCuartos);

    printf("\n==================================================\n");
    printf(" Piso %d de %d | Cuarto %d de %d\n",
        (*estado).pisoActual, (*estado).pisosTotales,
        (*estado).cuartoActual, totalCuartos);
    printf("==================================================\n");

    enemigo = seleccionarEnemigo(bestiario, total_enemigos, es_cuarto_final);
    original = enemigo;   /* copia intacta: el combate modifica la vida de enemigo */

    if(es_cuarto_final){
        printf(" Un enemigo RARO bloquea la salida del piso!\n");
    } else {
        printf(" Un enemigo ha aparecido en el camino!\n");
    }

    iniciarCombate(estado, &enemigo);

    if((*estado).hpActual > 0){
        if(enemigo.vidaBase <= 0){
            /* victoria en el combate */
            (*estado).enemigosDerrotados++;
            (*estado).cuartosExplorados++;
            (*estado).experiencia = (*estado).experiencia + calcularExperiencia(original);
            printf(" Ganas %d puntos de experiencia.\n", calcularExperiencia(original));
            verificarSubidaNivel(estado);
            otorgarRecompensa(estado, catalogo, total_objetos, original.tipo);
            revisarProgresoPiso(estado, totalCuartos);
        } else {
            /* el jugador huyo */
            (*estado).cuartoActual--;
            printf(" Escapas del cuarto, pero el camino sigue bloqueado.\n");
        }
    }
}

/* Si ya se completaron todos los cuartos del piso, desciende al
 * siguiente. Si no quedan mas pisos, anuncia la victoria (el estado
 * queda con pisoActual > pisosTotales). */
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

/* ===================================================================
 *  UTILIDAD DE ENTRADA
 * =================================================================== */

/* Pide un entero por consola hasta que este entre minimo y maximo
 * (inclusive). Ignora entradas que no sean numeros. Si la entrada se
 * termina (fin de archivo) cierra el programa para no ciclar. */
int leerOpcion(int minimo, int maximo){
    int opcion = minimo - 1;
    int leidos;

    do{
        printf(" > ");
        leidos = scanf("%d", &opcion);
        if(leidos == EOF){
            printf("\nEntrada finalizada. Saliendo.\n");
            exit(0);
        }
        if(leidos != 1){
            while(getchar() != '\n');   /* descarta la linea invalida */
            opcion = minimo - 1;
        }
        if(opcion < minimo || opcion > maximo){
            printf(" Ingresa un numero entre %d y %d\n", minimo, maximo);
        }
    } while(opcion < minimo || opcion > maximo);

    return opcion;
}

/* ===================================================================
 *  MODULO 3: SISTEMA DE COMBATE Y PROGRESION
 * =================================================================== */

int calcularDano(int ataque_atacante, int defensa_defensor){
    int dano = ataque_atacante - defensa_defensor;

    if(dano < 1){
        dano = 1;
    }
    return dano;
}

/* % de huir con exito. Parte de HUIDA_BASE y sube o baja HUIDA_POR_PUNTO por cada punto que la defensa del jugador supere
(o quede por debajo) del ataque del enemigo. */
int calcularProbabilidadHuida(int ataque_enemigo, int defensa_jugador){
    int prob = HUIDA_BASE + HUIDA_POR_PUNTO * (defensa_jugador - ataque_enemigo);

    if(prob < HUIDA_MIN){
        prob = HUIDA_MIN;
    }
    if(prob > HUIDA_MAX){
        prob = HUIDA_MAX;
    }
    return prob;
}

/* intenta escapar del combate. devuelve 1 si lo logra y 0 si falla. */
int intentarHuir(EstadoJuego *estado, Entidad *enemigo){
    int prob = calcularProbabilidadHuida((*enemigo).danoBase, (*estado).defensa);
    int exito = (rand() % 100) < prob;

    printf(" Intentas huir (%d%% de probabilidad)... ", prob);
    if(exito){
        printf("lo logras!\n");
    } else {
        printf("fallas!\n");
    }
    return exito;
}

/* el enemigo ataca al jugador. el HP del jugador nunca baja de 0. */
void turnoEnemigo(EstadoJuego *estado, Entidad *enemigo){
    int dano = calcularDano((*enemigo).danoBase, (*estado).defensa);

    (*estado).hpActual = (*estado).hpActual - dano;
    if((*estado).hpActual < 0){
        (*estado).hpActual = 0;
    }
    printf(" %s te ataca y te hace %d de daño.\n", (*enemigo).nombreEnemigo, dano);
}

/* combate por turnos:  cada accion del jugador (atacar, usar la mochila o intentar huir) consume el turno y luego ataca el enemigo, salvo que
este ya haya muerto o el jugador haya escapado. Entrar a la mochila y volver atras sin usar ni soltar nada no consume turno (se detecta
porque la cantidad de objetos no cambia). El combate termina cuando muere el jugador, muere el enemigo o el jugador huye. */
void iniciarCombate(EstadoJuego *estado, Entidad *enemigo){
    int huyo = 0;
    int opcion, dano, objetosAntes, turnoConsumido;

    printf("\n *** COMBATE contra %s (%s) ***\n",
        (*enemigo).nombreEnemigo, (*enemigo).tipo);

    while((*estado).hpActual > 0 && (*enemigo).vidaBase > 0 && !huyo){
        printf("\n %s: %d HP | Tu: %d/%d HP (Nivel %d)\n",
            (*enemigo).nombreEnemigo, (*enemigo).vidaBase,
            (*estado).hpActual, (*estado).hpLimite, (*estado).nivel);
        printf(" (1) Atacar\n (2) Mochila\n (3) Huir\n");
        opcion = leerOpcion(1, 3);
        turnoConsumido = 1;

        if(opcion == 1){
            dano = calcularDano((*estado).ataque, (*enemigo).defensaBase);
            (*enemigo).vidaBase = (*enemigo).vidaBase - dano;
            if((*enemigo).vidaBase < 0){
                (*enemigo).vidaBase = 0;
            }
            printf(" Atacas a %s y le haces %d de dano.\n", (*enemigo).nombreEnemigo, dano);
        } else if(opcion == 2){
            objetosAntes = (*estado).espacioInventarioActual;
            usarObjeto(estado);
            turnoConsumido = ((*estado).espacioInventarioActual != objetosAntes);
        } else {
            huyo = intentarHuir(estado, enemigo);
        }

        if(turnoConsumido && !huyo && (*enemigo).vidaBase > 0){
            turnoEnemigo(estado, enemigo);
        }
    }

    if((*estado).hpActual <= 0){
        printf("\n Has caido en combate...\n");
    } else if((*enemigo).vidaBase <= 0){
        printf("\n Has derrotado a %s!\n", (*enemigo).nombreEnemigo);
    }
}

/* XP que da un enemigo: la mitad de su vida base mas su dano base. Se calcula con la copia original del enemigo (antes del combate). */
int calcularExperiencia(Entidad enemigo){
    return enemigo.vidaBase / 2 + enemigo.danoBase;
}

/* XP acumulada necesaria para pasar del nivel indicado al siguiente. */
int umbralExperiencia(int nivel){
    return XP_BASE_NIVEL * nivel;
}

/* Sube de nivel mientras la experiencia alcance el umbral (puede subir varios niveles seguidos). El sobrante de XP se conserva. Cada nivel
aumenta HP maximo, ataque y defensa, y cura lo que sube el HP maximo. */
void verificarSubidaNivel(EstadoJuego *estado){
    while((*estado).experiencia >= umbralExperiencia((*estado).nivel)){
        (*estado).experiencia = (*estado).experiencia - umbralExperiencia((*estado).nivel);
        (*estado).nivel++;
        (*estado).hpLimite = (*estado).hpLimite + BONO_HP;
        (*estado).hpActual = (*estado).hpActual + BONO_HP;
        (*estado).ataque = (*estado).ataque + BONO_ATAQUE;
        (*estado).defensa = (*estado).defensa + BONO_DEFENSA;
        printf(" SUBES AL NIVEL %d! (HP max %d | ATK %d | DEF %d)\n",
            (*estado).nivel, (*estado).hpLimite, (*estado).ataque, (*estado).defensa);
    }
}

/* Botin tras ganar un combate: un enemigo Raro siempre suelta un objeto (100%); uno Comun lo suelta con PROB_DROP_COMUN %. El objeto
se elige al azar del catalogo. Si la mochila esta llena, el objeto se pierde
(luego intentare que este pueda elegir reemplazarlo por alguno, pero eso lo veo luego xd) */
void otorgarRecompensa(EstadoJuego *estado, Objeto catalogo[], int total_objetos, char tipo_enemigo[]){
    int suelta, indice;

    if(strcmp(tipo_enemigo, "Raro") == 0){
        suelta = 1;
    } else {
        suelta = (rand() % 100) < PROB_DROP_COMUN;
    }

    if(suelta && total_objetos > 0){
        indice = rand() % total_objetos;
        if((*estado).espacioInventarioActual < (*estado).espacioInventarioLimite){
            (*estado).mochila[(*estado).espacioInventarioActual] = catalogo[indice];
            (*estado).espacioInventarioActual++;
            printf(" El enemigo solto un objeto: %s (mochila %d/%d)\n",
                catalogo[indice].nombreObjeto,
                (*estado).espacioInventarioActual, (*estado).espacioInventarioLimite);
        } else {
            printf(" El enemigo solto %s, pero tu mochila esta llena y se pierde.\n",
                catalogo[indice].nombreObjeto);
        }
    }
}

/* ===================================================================
 *  MODULO 4: GESTION DE RECURSOS (PROVISIONAL)
 * =================================================================== */

/* Version provisional: solo informa el contenido de la mochila sin usar ni soltar nada. Se reemplaza en el Bloque 4.*/
void usarObjeto(EstadoJuego *estado){
    int i;

    printf(" Mochila (%d/%d):\n", (*estado).espacioInventarioActual,
        (*estado).espacioInventarioLimite);
    for(i = 0; i < (*estado).espacioInventarioActual; i++){
        printf("   %d. %s\n", i + 1, (*estado).mochila[i].nombreObjeto);
    }
    printf(" (Usar y soltar objetos se implementa en el Bloque 4)\n");
}
