# Overtime Tracker

Sistema gerenciador de horas extras desenvolvido em C com interface gráfica GTK4 e banco de dados MySQL.

## Sobre

O sistema exibe uma tela de autenticação que solicita nome de usuário e senha. Ao submeter:
- **Se os campos estiverem vazios**: Exibe mensagem de erro;
- **Se as credenciais forem inválidas**: Exibe mensagem de erro;
- **Se as credenciais forem válidas**: Fecha a janela de login e encerra.

### Funcionalidades
- Sistema de autenticação de usuários;
- Conexão com banco de dados MySQL;
- Interface gráfica com GTK4;
- Validação de campos vazios;
- Verificação de credenciais no banco de dados.

## Configuração do Ambiente de Desenvolvimento

### Pré-requisitos

1. **Compilador C**
   - [Guia de instalação](https://syntaxpathways.com/set-up-c-development-environment/)

2. **IDE Code::Blocks 25.03**
   - [Download aqui](https://sourceforge.net/projects/codeblocks/files/Binaries/25.03/Windows/codeblocks-25.03-setup.exe/download)

3. **MySQL Server**
   - [Download aqui](https://dev.mysql.com/downloads/windows/installer/8.0.html)

> **Observação**: Instale todos os programas nos diretórios padrão. Caso contrário, será necessário configurar os caminhos manualmente na IDE.

## Instalação das Dependências

### 1. GTK4 e Utilitários

Abra o terminal **MSYS2 MSYS** e execute:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-toolchain
pacman -S base-devel
pacman -S mingw-w64-x86_64-gtk4
pacman -S mingw-w64-x86_64-glade
pacman -Syu
```

Verifique se instalação correu bem executando o seguinte comando no terminal **MSYS2 MINGW64**:

```bash
pkg-config --cflags gtk4
```

> **Observação**: Caso o comando não seja encontrado, repita os passos de instalação novamente.

## Configuração do Banco de Dados

1. Abra o **MySQL Command Line Client**
2. Faça login com a senha do usuário `root`
3. Execute o arquivo de inicialização:

```sql
source C:\Users\SeuUsuario\caminho\para\o\projeto\init.sql
```

## Compilação e Execução

1. Apartir do Code::Blocks, abra o arquivo `Overtime Tracker.cbp` no Code::Blocks
2. Compile o projeto: `Ctrl + F9`
3. Aguarde a mensagem de sucesso na saída
4. Execute o programa: `Ctrl + F10`
5. Teste o cliente com as credenciais:
- **Usuário**: `admin`
- **Senha**: `1234`

### TO-DO
- Implementar painel principal após login
- Cadastro de horas extras
- Relatórios e consultas
- Gestão de usuários

## Referências

- [Instalação do GNU COMPILER COLLECTION](https://syntaxpathways.com/set-up-c-development-environment/)
- [Instalação e configuração da biblioteca GTK4 na IDE Code::Blocks](https://www.treinaweb.com.br/blog/criando-interfaces-graficas-no-c-com-gtk/)
- [Instalação e configuração do connector MySQL na IDE Code::Blocks](https://www.treinaweb.com.br/blog/utilizando-o-mysql-em-uma-aplicacao-c/)
- [Instalação das dependências de compilação do GTK4](https://www.treinaweb.com.br/blog/criando-uma-aplicacao-c-com-gtk-no-codeblocks/)
- [Documentação da biblioteca GTK4](https://docs.gtk.org/gtk4/)
- [Utilização da linha do comando do Cliente MySQL](https://www.treinaweb.com.br/blog/primeiros-passos-com-mysql)
