/*
 * GAME SCENE
 * Copyright © 2018+ Ángel Rodríguez Ballesteros
 *
 * Distributed under the Boost Software License, version  1.0
 * See documents/LICENSE.TXT or www.boost.org/LICENSE_1_0.txt
 *
 * angel.rodriguez@esne.edu
 */

#include "Game_Scene.hpp"
#include "Menu_Scene.hpp"
#include "Game_Over_Scene.hpp"

#include <cstdlib>
#include <basics/Canvas>
#include <basics/Director>

using namespace basics;
using namespace std;

namespace example
{
    // ---------------------------------------------------------------------------------------------
    // ID y ruta de las texturas que se deben cargar para esta escena. La textura con el mensaje de
    // carga está la primera para poder dibujarla cuanto antes:

    Game_Scene::Texture_Data Game_Scene::textures_data[] =
    {
        { ID(loading),        "loading.png"       },
        { ID(player),         "player.png"        },
        { ID(p-button),       "pause-button.png"  },
        { ID(enemy),          "enemy.png"         },
        { ID(instructions-t), "controls.png"      },

    };

    // Pâra determinar el número de items en el array textures_data, se divide el tamaño en bytes
    // del array completo entre el tamaño en bytes de un item:

    unsigned Game_Scene::textures_count = sizeof(textures_data) / sizeof(Texture_Data);

    // ---------------------------------------------------------------------------------------------
    // Definiciones de los atributos estáticos de la clase:

    constexpr float Game_Scene::player_speed;
    constexpr float Game_Scene::enemy_speed;
    constexpr int Game_Scene::playerLives;


    // ---------------------------------------------------------------------------------------------

    Game_Scene::Game_Scene()
    {
        // Se establece la resolución virtual (independiente de la resolución virtual del dispositivo).
        // En este caso no se hace ajuste de aspect ratio, por lo que puede haber distorsión cuando
        // el aspect ratio real de la pantalla del dispositivo es distinto.

        canvas_width  = 720;
        canvas_height =  1280;

        // Se inicia la semilla del generador de números aleatorios:

        srand (unsigned(time(nullptr)));

        // Se inicializan otros atributos:

        initialize ();
    }

    // ---------------------------------------------------------------------------------------------
    // Algunos atributos se inicializan en este método en lugar de hacerlo en el constructor porque
    // este método puede ser llamado más veces para restablecer el estado de la escena y el constructor
    // solo se invoca una vez.

    bool Game_Scene::initialize ()
    {
        state     = LOADING;
        suspended = true;
        gameplay  = UNINITIALIZED;

        return true;
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::suspend ()
    {
        suspended = true;               // Se marca que la escena ha pasado a primer plano
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::resume ()
    {
        suspended = false;              // Se marca que la escena ha pasado a segundo plano
    }

    // ---------------------------------------------------------------------------------------------

    bool timerReset = false;
    void Game_Scene::handle (Event & event)
    {
        if (state == RUNNING)               // Se descartan los eventos cuando la escena está LOADING
        {
            if (gameplay == WAITING_TO_START)
            {
                start_playing ();           // Se empieza a jugar cuando el usuario toca la pantalla por primera vez
            }
            else switch (event.id)
            {
                case ID(touch-started):     // El usuario toca la pantalla
                case ID(touch-moved):
                {
                    user_target_y = *event[ID(y)].as< var::Float > ();
                    user_target_x = *event[ID(x)].as< var::Float > ();
                    follow_target = true;
                    break;
                }

                case ID(touch-ended):       // El usuario deja de tocar la pantalla
                {
                    follow_target = false;
                    timerReset =false;
                    break;
                }
            }
        }
    }

    // ---------------------------------------------------------------------------------------------

    /**
     * Método update con diferentes casos, si las texturas han sido cargadas, se inicia la lógica del juego
     * @param time
     */
    void Game_Scene::update (float time)
    {
        if (!suspended) switch (state)
        {
            case LOADING: load_textures  ();     break;
            case RUNNING: run_simulation (time); break;
            case ERROR:   break;
        }


    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::render (Context & context)
    {
        if (!suspended)
        {
            // El canvas se puede haber creado previamente, en cuyo caso solo hay que pedirlo:

            Canvas * canvas = context->get_renderer< Canvas > (ID(canvas));

            // Si no se ha creado previamente, hay que crearlo una vez:

            if (!canvas)
            {
                 canvas = Canvas::create (ID(canvas), context, {{ canvas_width, canvas_height }});
            }

            // Si el canvas se ha podido obtener o crear, se puede dibujar con él:

            if (canvas)
            {
                canvas->clear ();

                switch (state)
                {
                    case LOADING: render_loading   (*canvas); break;
                    case RUNNING: render_playfield (*canvas); break;
                    case ERROR:   break;
                }
            }
        }
    }

    // ---------------------------------------------------------------------------------------------
    // En este método solo se carga una textura por fotograma para poder pausar la carga si el
    // juego pasa a segundo plano inesperadamente. Otro aspecto interesante es que la carga no
    // comienza hasta que la escena se inicia para así tener la posibilidad de mostrar al usuario
    // que la carga está en curso en lugar de tener una pantalla en negro que no responde durante
    // un tiempo.

    void Game_Scene::load_textures ()
    {
        if (textures.size () < textures_count)          // Si quedan texturas por cargar...
        {
            // Las texturas se cargan y se suben al contexto gráfico, por lo que es necesario disponer
            // de uno:

            Graphics_Context::Accessor context = director.lock_graphics_context ();

            if (context)
            {
                // Se carga la siguiente textura (textures.size() indica cuántas llevamos cargadas):

                Texture_Data   & texture_data = textures_data[textures.size ()];
                Texture_Handle & texture      = textures[texture_data.id] = Texture_2D::create (texture_data.id, context, texture_data.path);

                // Se comprueba si la textura se ha podido cargar correctamente:

                if (texture) context->add (texture); else state = ERROR;

                // Cuando se han terminado de cargar todas las texturas se pueden crear los sprites que
                // las usarán e iniciar el juego:
            }
        }
        else
        if (timer.get_elapsed_seconds () > 1.f)         // Si las texturas se han cargado muy rápido
        {                                               // se espera un segundo desde el inicio de
            create_sprites ();                          // la carga antes de pasar al juego para que
            restart_game   ();                          // el mensaje de carga no aparezca y desaparezca
                                                        // demasiado rápido.
            state = RUNNING;
        }
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::create_sprites ()
    {


        //Se crean los enemigos
        Sprite_Handle enemies(new Sprite(textures[ID(enemy)].get())); //utilizado para obtener las medidas despues del escalado


        // Se crean las intrucciones
        Sprite_Handle instructionsH(new Sprite( textures[ID(instructions-t)].get()));

        //Se crea el botón de pause
        Sprite_Handle pauseButton(new Sprite( textures[ID(p-button)].get()));


        //Se calcula la escala
        float scale = 1.0f;
        generalScale = scale;


        //Se crea el jugador
        Sprite_Handle playerHandle(new Sprite(textures[ID(player)].get()));

        // Se incluyen los sprites a su lista para ser renderizados
        sprites.push_back (playerHandle);
        sprites.push_back (pauseButton);

        // Se guardan punteros a los sprites que se van a usar frecuentemente:

        player  = playerHandle.get ();
        buttonPause = pauseButton.get();
        instructions = instructionsH.get();

        //Se crean los objetos del HUD
        buttonPause->set_position({get_view_size().width-150, get_view_size().height-100});
        instructions->set_position({get_view_size().width/2,get_view_size().height/2});

    }


    /**
     * Método que a partir de sus muchos parámetros, crea una lista de sprites iguales, puedes incluir separaciones, puntos iniciales, varias texturas en función de su orientación...
     * @param initialPositionY
     * @param thisObjectHeight
     * @param list
     * @param horizontal_separation
     * @param vertical_separation
     * @param verticalCorrection
     * @param numberVertical
     * @param numberHorizontal
     * @param texture2D
     * @param texture2D1
     */
    void Game_Scene::fillCustomList(const float & initialPositionY, const float & thisObjectHeight, std::vector<Sprite *> & list,
                                    const float horizontal_separation,const float vertical_separation , const float verticalCorrection, const float numberVertical ,
                                    const float numberHorizontal, Texture_2D * texture2D, Texture_2D * texture2D1) {

    }

    /**
     * Método que devuelve la altura inicial y final reescalada.
     * @param sprite
     * @param initialFinal
     * @param scale
     * @return
     */
    float Game_Scene::GetInitialOrFinalScaledHeights(const Sprite & sprite , const bool initialFinal, const float scale) const {

        if(scale == 1.0){
            if(!initialFinal){
                return 0.0;
            }
            else return sprite.get_height();
        }

        else{
            float initialHeight = (sprite.get_height()*scale)/2  ;
            if(!initialFinal){
                return initialHeight;
            }
            else return  initialHeight + sprite.get_height()/2*scale;
        }
    }


    // ---------------------------------------------------------------------------------------------
    // Juando el juego se inicia por primera vez o cuando se reinicia porque un jugador pierde, se
    // llama a este método para restablecer la posición y velocidad de los sprites:

    void Game_Scene::restart_game()
    {
        player->set_scale(generalScale);
        player->set_position ({canvas_width/2.f + player->get_height()*generalScale / 2, player->get_height()*generalScale / 2});
        player->set_speed_y  (0.f);
        player->set_speed_x (0.f);


        follow_target = false;

        gameplay = WAITING_TO_START;
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::start_playing ()
    {
        gameplay = PLAYING;
    }

    // ---------------------------------------------------------------------------------------------
    /**
     * Método que incluye toda la lógica del juego, ésta se pausara si se toca el botón de pausa, y se despausará al volverla a tocar.
     * @param time
     */
    void Game_Scene::run_simulation (float time)
    {
        // Se actualiza el estado de todos los sprites:


        if(!paused){
            if(buttonPause->contains({user_target_x,user_target_y})){
                paused = true;
                user_target_x = 0;
                user_target_y = 0;
            }

            update_user ();

            if(currentLives == playerLives) director.run_scene (shared_ptr< Scene >(new Game_Over_Scene)); // Si el jugador muere 3 veces, se cambia de escena a la de game over


            for (auto & sprite : sprites)
            {
                sprite->update (time);
            }


        }

        else{

            if(buttonPause->contains({user_target_x,user_target_y})){
                paused = false;
                user_target_x = 0;
                user_target_y = 0;
            }

        }




        // Se comprueban las posibles colisiones de la bola con los bordes y con los players:

        check_collisions();

        // Si se llega al final, se sale al menú, habiendo ganado
        if (gameplay == BALL_LEAVING) director.run_scene(shared_ptr< Scene >(new Menu_Scene));
    }

    // ---------------------------------------------------------------------------------------------
    // Se hace que el player dechero se mueva hacia los lados según el usuario esté
    // tocando la pantalla por la derecha o por la izquierda de su centro.
    // Cuando el usuario no toca la pantalla la nave no se mueve.

    bool move = true;

    /**
     * Actualiza al jugador, el movimiento se realiza por intervalos de tiempo y velocidad, a modo de salto.
     */
    void Game_Scene::update_user ()                                                                                                 //Actualiza el jugador
    {
        if(follow_target){
            if(move){
                Player_Movement(); // Si el jugador pulsa la pantalla, se mueve
            }
        }

        // Se crea el intervalo de parada de movimiento, habiendo pulsado una vez, se tarda 300 milésimas de segundo en parar y se mueve durante 200
        if(timer.get_elapsed_seconds()>=.2f){
            move = false;
            player->set_speed({0,0});

            if(timer.get_elapsed_seconds()>=0.3f){
                move = true;
                timerReset=false;
                timer.reset();
            }
        }

    }
    // ---------------------------------------------------------------------------------------------

    /**
     * Comprueba las colisiones a lo bruto, todos con todos.
     */
    void Game_Scene::check_collisions()                                                                                               // Comprueba las colisiones
    {
        /*for(auto sprite : enemy){
            // Si el jugador choca contra uno de los enemigos se le quita 1 vida.
            if(sprite->intersects(*player,generalScale)){
                restart_game();
                currentLives++;
            }
        }*/

    }

    // ---------------------------------------------------------------------------------------------



    void Game_Scene::render_loading (Canvas & canvas)
    {
        Texture_2D * loading_texture = textures[ID(loading)].get ();

        if (loading_texture)
        {
            canvas.fill_rectangle
            (
                { canvas_width * .5f, canvas_height * .5f },
                { loading_texture->get_width (), loading_texture->get_height ()},
                  loading_texture
            );
        }
    }

    // ---------------------------------------------------------------------------------------------
    // Simplemente se dibujan todos los sprites que conforman la escena.

    void Game_Scene::render_playfield (Canvas & canvas)                                                                         // Se dibujan todos los sprites de la lista
    {
        if(!paused){
            for (auto & sprite : sprites)
            {
                sprite->render (canvas);
            }

            /*for(auto & sprite : enemies){
                sprite->render(canvas);
            }*/

            player->render(canvas);

        }
        else instructions->render(canvas);
        buttonPause->render(canvas);


    }


    /**
     * Método que mueve al jugador en la dirección deseada
     */
    void Game_Scene::Player_Movement(){

        if(!timerReset){
            timerReset=true;
            move=true;
            timer.reset();

        }

        // En función de donde toca el usuario, se móvera en una dirección o disparará.
        // Izquierda: Pulsando el 1/3 de la pantalla (x)
        // Derecha:Pulsando el 3/3 de la pantalla (x)
        // Disparar : Pulsando el 2/3 de la pantalla (X)
        if(user_target_x <= get_view_size().width/3){

            if(player->get_position_x()>= player->get_width()*generalScale){
                player->set_speed_x(-player_speed);
            }
            else player->set_speed({0,0});

        }
        else if (user_target_x >= (get_view_size().width *2 )/3 ){

            if(player->get_position_x()<= get_view_size().width - player->get_width()*generalScale){
                player->set_speed_x(player_speed);
            }
            else player->set_speed({0,0});
        }

        else if (user_target_x > get_view_size().width/3 && user_target_x < (get_view_size().width *2 )/3 ){

            //AQUI VA LA FUNCIÓN DE DISPARO
        }

        else{}

    }
}