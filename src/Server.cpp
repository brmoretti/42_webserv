/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bmoretti <bmoretti@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/27 10:40:35 by bmoretti          #+#    #+#             */
/*   Updated: 2024/07/02 11:52:58 by bmoretti         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

Server::Server(const std::string &address, int port) : _address(address),
  _port(port), _server_fd(-1), _epoll_fd(-1)
{
  _initServer();
}

Server::~Server()
{
  /* verificar se o os fds sao diferentes de -1 para dar close */
  if (_server_fd != -1)
  {
    close(_server_fd);
  }

  if (_epoll_fd != -1)
  {
    close(_epoll_fd);
  }

}
void Server::_initServer()
{
  /*   criando o socket/fd e verificando se ele continua -1 (se continuar, erro) */
	this->_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_server_fd == -1) {
		throw std::runtime_error("Failed to create socket");
	}

  /* configurando o socket/fd do servidor para poder ser reutilizado assim que
   * for fechado, eh bom para casos em que voce precisa reiniciar o servidor
   * sem esperar em que o sistema operacional liberar a porta automaticamente
   * (normalmente tem um tempo para isso ocorrer)
   **/
  int opt = 1; // argumento para ser usado com o novo valor para setar.
  if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
  {
    throw std::runtime_error("Failed to set socket options");
  }

  /* estrutura para usar com o protocolo IPv4 e contem campos para especificar o endereco de IP
   * numero da porta dentro outras coisas.
  **/
  sockaddr_in server_addr;

  /* inicializando todo os bytes para do server_addr para 0 para assegurar que todos os campos vao inicar
  * com o mesmo valor
  **/
  memset(&server_addr, 0, sizeof(server_addr));

  /* sin_famlily = Address Family Internet - sever para especificar que esta usando IPV4 */
  server_addr.sin_family = AF_INET;
  
  /* a funcao inet_addr recebe uma representacao de uma string(char *) com um endereco IPv4 e retorna
   * a representacao binaria do ip para operacoes de rede.
   * e essa linha so esta setando o endereco de ip para o ip passado via string para a inet_addr
   **/
  server_addr.sin_addr.s_addr = inet_addr(_address.c_str());

  /* variavel da struct server_addr em que voce indica a porta que sera utilizada
   * a funcao htons ("Host TO Network Short") converte o numero da porta para bytes de ordem de host
   * ela garante que o numero da porta estara corretamente interpretado independente da arquitetura
   * do sistema.
   * */
  server_addr.sin_port = htons(_port);

  /* a funcao bind associa o socket referido pelo fd '_server_fd' com o endereco de IP local
   * e com a porta especificada na estrutura 'server_addr' e informa ao sistema que  a aplicacao
   * quer receber dados no ip e na porta direcionada.
   **/
  if (bind(_server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1)
  {
    throw std::runtime_error("Failed to bind socket");
  }

  /* apos o binding a funcao listen server parr colocar o socket no estado de espera.
   * Significa que o socket esta pronto para aceitar as requisicoes de conexao de
   * entrada dos clientes.
   **/
  if (listen(_server_fd, MAX_CONNECTIONS) == -1)
  {
    throw std::runtime_error("Failed to listen socket");
  }

  /* maiores explicacoes dentro da funcao principal
   * mas a funcao apenas captura e altera as flags do fd recebido
   * como parametro
   **/
  setNonBlocking(this->_server_fd);

  /* epoll_create1 eh mais simples que a epoll_create, ela recebe apenas a flags como parametro.
   * ela eh usada para iniciar a interface de fds, que eh um mecanismo para monitorar multiplos fds
   * epoll_create recebe o tamanho (quantidade de fds) que ira monitorar, porem a '1' nao precisa
   * porque ela adiciona dinamicamente.
   **/
  this->_epoll_fd = epoll_create1(0);
  if (this->_epoll_fd == -1)
  {
    throw std::runtime_error("Failed to create epoll file descriptor");
  }

  /* essa parte do codigo configura um evento 'epoll'  para monitorar um socket '_server_fd'
   * em busca de novas conexoes. Ele utiliza um mecanismo de notificacao de entrada e saida
   * escalavel
   * primeiro criamos a variavel que vai monitorar.
   **/
  epoll_event event;

  /* configurando o evento
   * EPOLLIN - Indica que o evento pode ser adicionado quando houver dados disponiveis
   * para leitura no fd. Normalmente significa que um novo pedido de conexao chegou.
   * EPOLLET - Esse eh acionado uma vez por serie de eventos.
   **/
  event.events = EPOLLIN | EPOLLET;

  /* associando o fd/socket que sera monitorado */
  event.data.fd = this->_server_fd;

  /* epoll_ctl eh chamada para adicionar, modificar ou deletar eventos na instancia epoll
   * atrelada ao _server_fd
   **/
  if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server_fd, &event) == -1)
  {
    throw std::runtime_error("Failed to add server socket to epoll");
  }
}

/* essa funcao 'run' foi feita para esperar eventos de varios clients (fds) de maneira eficiente
 * ela monitora a instancia do epoll_fd permitindo que o sevidor esperar atividade de varios fds
 **/
void  Server::run()
{
  /* esse eh o loop geral para o servidor ficar esperando por eventos */
  while (true)
  {
    /* essa funcao espera por eventos de fds registrados na instancia epoll determinada
     * na _epoll_fd
     **/
    int event_count = epoll_wait(_epoll_fd, _events, MAX_EVENTS, -1);
    if (event_count == -1)
    {
      throw std::runtime_error("Failed to wait on epoll");
    }

    /* esse loop eh para processamento do eventos detectados */
    for (int i = 0; i < event_count; i++)
    {
      /* aqui ele ficar verificando se o evento pertence ao fd do server
       * ou do cliente ja conectado.
       * Se for do servidor, ele tentara aceitar novas conexoes em um loop
       * e para cada nova conexao ele configura o fd do cliente para o modo
       * nao bloqueante e registra o descritor na epoll para futuras operacoes
       * de leitura.
       **/
      if (_events[i].data.fd == _server_fd)
      {
        /* configurando o fd do cliente e aceitando a conexao */
        while (true)
        {
          sockaddr_in client_addr;
          socklen_t client_addr_len = sizeof(client_addr);
          int client_fd = accept(_server_fd, (sockaddr*)&client_addr, &client_addr_len);
          if (client_fd == -1)
          {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
              break;
            else
            {
              std::cerr << "Failed to accept client connection" << std::endl;
              break;
            }
          }
          setNonBlocking(client_fd);

          epoll_event event;
          event.events = EPOLLIN | EPOLLET;
          event.data.fd = client_fd;
          if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
          {
            std::cerr << "Failed to add client socket to epoll" << std::endl;
            close(client_fd);
          }
        }
      } else {
        handleConnection(_events[i].data.fd); // Se o evento nao for do servidor
        // e sim um cliente ja conectado, essa funcao eh chamada para processar os dados recebidos
        // desse cliente
      }
    }
  }
}

void Server::handleConnection(int client_fd)
{
  char buffer[BUFFER_SIZE];
  while (true)
  {
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break ; // no more data to read
      else
      {
        std::cerr << "Read error" << std::endl;
        close(client_fd);
        break ;
      }
    }
    else if (bytes_read == 0)
    {
      close(client_fd);
      break ; // client closed connection
    }
    else {
      buffer[bytes_read] = '\0';
      // std::cout << "From client: " << client_fd << " | Received: " << buffer;

      const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
      ssize_t bytes_written = write(client_fd, response, strlen(response));
      if (bytes_written == -1)
      {
        std::cerr << "Write error" << std::endl;
      }
      // close(client_fd); // descomentar para ter apenas uma 'requisicao e fechar a conexao
      // acho que isso nao vai precisar, provavelmente vamos precisar lidar com sinais.
    }
  }
}

void Server::setNonBlocking(int fd)
{
  /* essa funcao eh usada para manipular as flags dos files descriptors.
   * nessa parte do codigo estamos usando para pegar as atuais flags do
   * fd passado no primero parametro.
   **/
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
  {
    throw std::runtime_error("Failed to get file descriptor flags");
  }

  /* essa parte eh para adicionar a flag non-blocking ao set de flags existente do fd. */
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
  {
    throw std::runtime_error("Failed to set non-blocking mode");
  }
}
