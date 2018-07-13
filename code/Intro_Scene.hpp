/*
 * INTRO SCENE
 * Copyright © 2018+ Ángel Rodríguez Ballesteros
 *
 * Distributed under the Boost Software License, version  1.0
 * See documents/LICENSE.TXT or www.boost.org/LICENSE_1_0.txt
 *
 * angel.rodriguez@esne.edu
 */

#ifndef INTRO_SCENE_HEADER
#define INTRO_SCENE_HEADER

    #include <memory>
    #include <basics/Canvas>
    #include <basics/Scene>
    #include <basics/Texture_2D>
    #include <basics/Timer>

    namespace example
    {

        using basics::Timer;
        using basics::Canvas;
        using basics::Texture_2D;
        using basics::Graphics_Context;

        class Intro_Scene : public basics::Scene
        {

            /**
             * Representa el estado de la escena en su conjunto.
             */
        protected:
            enum State
            {
                UNINITIALIZED,
                LOADING,
                FADING_IN,
                WAITING,
                FADING_OUT,
                FINISHED,
                ERROR
            };

        protected:

            State    state;                                     ///< Estado de la escena.
            bool     suspended;                                 ///< true cuando la escena está en segundo plano y viceversa.

            unsigned canvas_width;                              ///< Ancho de la resolución virtual usada para dibujar.
            unsigned canvas_height;                             ///< Alto  de la resolución virtual usada para dibujar.

            Timer    timer;                                     ///< Cronómetro usado para medir intervalos de tiempo.

            float    opacity;
            ///< Opacidad de la textura.
            float          x, y;

            std::shared_ptr < Texture_2D > logo_texture;        ///< Textura que contiene la imagen del logo.

        public:

            Intro_Scene()
            {
                state         = UNINITIALIZED;
                suspended     = true;
                canvas_width  = 720;
                canvas_height = 1280;
                x = -10;
                y = -10;
            }

            /**
             * Este método lo llama Director para conocer la resolución virtual con la que está
             * trabajando la escena.
             * @return Tamaño en coordenadas virtuales que está usando la escena.
             */
            basics::Size2u get_view_size () override
            {
                return { canvas_width, canvas_height };
            }

            /**
             * Aquí se inicializan los atributos que deben restablecerse cada vez que se inicia la escena.
             * @return
             */
            bool initialize () override;

            /**
             * Este método lo invoca Director automáticamente cuando el juego pasa a segundo plano.
             */
            void suspend () override
            {
                suspended = true;
            }

            /**
             * Este método lo invoca Director automáticamente cuando el juego pasa a primer plano.
             */
            void resume () override
            {
                suspended = false;
            }

            /**
             * Este método se invoca automáticamente una vez por fotograma para que la escena
             * actualize su estado.
             */
            void update (float time) override;

            /**
             * Este método se invoca automáticamente una vez por fotograma para que la escena
             * dibuje su contenido.
             */
            void render (Graphics_Context::Accessor & context) override;

        protected:

            virtual void update_loading    ();
            void update_fading_in  ();
            virtual void update_waiting    ();
            virtual void update_fading_out ();
            virtual void Render_Textures(Canvas & canvas);

        };

    }

#endif
