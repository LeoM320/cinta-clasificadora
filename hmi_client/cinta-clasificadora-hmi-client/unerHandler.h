    #ifndef UNER_HANDLER_H
    #define UNER_HANDLER_H

    #include <cstdint>

    /**
     * @brief Definición de los limites de memoria.
     * Siempre debe ser un múltiplo de 8
     * Se relaciona directamente con la memoria dinámica ocupada
     * por lo que se debe ser cauto.
     */
    #define BUFSIZE 1024
    #define BUFLIMIT (BUFSIZE - 1)

    /**
     * @brief Definición del tiempo de refresco en la decodificación.
     * Si pasara esta cantidad de milisegundos sin nuevos datos,
     * se comenzará buscando el encabezado otra vez.
     */
    #define REFRESH 70

    class UnerHandler {
        /**
         * @brief Tipo de estructura utilizada por los procesos de Transmisión
         * de datos.
         * Con ella se establece un buffer circular de BUFSIZE bytes.
         */
        typedef struct {
            uint8_t iR;           //!< Índice de lectura.
            uint8_t iW;           //!< Índice de escritura.
            uint8_t buf[BUFSIZE]; //!< Buffer de transmisión.
            uint8_t checksum;     //!< Checksum generado.
        } _varTx;

        /**
         * @brief Tipo de enumeración que establece las etapas en la decodificación
         * de los datos.
         */
        typedef enum {
            U = 0,
            N,
            E,
            R,
            LENGTH,
            TOKEN,
            PAYLOAD,
            CHECKSUM
        } _eDecode;

        /**
         * @brief Tipo de estructura utilizada por los procesos de recepción
         * de datos.
         * Con ella se establece un buffer circular de BUFSIZE bytes.
         */
        typedef struct {
            uint8_t iR;                  //!< Índice de lectura.
            uint8_t iW;                  //!< Índice de escritura.
            uint8_t buf[BUFSIZE];        //!< Buffer de recepción.
            uint8_t checksum;            //!< Checksum generado.
            uint8_t length;              //!< Longitud del payload.
            uint8_t payload[BUFSIZE];    //!< Payload recibido.
            uint8_t payloadCount;        //!< Contador secuencial.
            _eDecode state;
        } _varRx;

        /**
         * @brief Tipo de unión utilizada para manipulación de datos tipo uint16_t.
         */
        typedef union {
            uint16_t valor;
            uint8_t byte[2];
        } _u16;

        /**
         * @brief Tipo de unión utilizada para manipulación de datos tipo uint32_t.
         */
        typedef union {
            uint32_t valor;
            uint8_t byte[4];
        } _u32;

        /**
         * @brief Tipo de unión utilizada para manipulación de datos tipo float.
         */
        typedef union {
            float valor;
            uint8_t byte[4];
        } _uF;

    public:
        UnerHandler();
        virtual ~UnerHandler() = default;

        void AbrirCarga(uint8_t length);
        void AgregarDato(uint8_t valor);
        void AgregarDato(uint16_t valor);
        void AgregarDato(uint32_t valor);
        void AgregarDato(float valor);
        void AgregarDato(uint8_t *valor, uint8_t length);
        void CerrarCarga();

        virtual uint8_t writeable() = 0;
        virtual void sendByte(uint8_t c) = 0;
        virtual int32_t readMs() = 0;
        virtual uint8_t readable() = 0;
        virtual uint8_t readByte() = 0;

        void Transmitir();
        void Recibir();
        void Decodificar();

        uint8_t Comando();
        uint8_t IDComando();
        uint8_t ObtenerUint8_t(uint8_t pos);
        uint16_t ObtenerUint16_t(uint8_t pos);
        uint32_t ObtenerUint32_t(uint8_t pos);
        float ObtenerFloat(uint8_t pos);

        virtual void EnviarBufTx(); // Virtual para poder optimizarlo en la PC
        void EnviarBufRx();

    protected: // Cambiado a protected para que las clases hijas accedan a los buffers
        uint32_t reset;     //!< Tiempo entre lectura de bytes.
        _varTx varTx;       //!< Variables de transmisión.
        _varRx varRx;       //!< Variables de recepción.
        _u16 u16;           //!< Unión de decodificación uint16_t.
        _u32 u32;           //!< Unión de decodificación uint32_t.
        _uF uF;             //!< Unión de decodificación float.
        uint8_t comando=0;  //!< Bandera de llegada de comando.
    };

    #endif
