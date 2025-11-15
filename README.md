# Overtime Tracker

Sistema gerenciador de horas extras desenvolvido em C com interface gráfica GTK4 e banco de dados MySQL.

## 📋 Status do Projeto

### Funcionalidades Implementadas
- ✅ Sistema de autenticação de usuários
- ✅ Conexão com banco de dados MySQL
- ✅ Interface gráfica com GTK4
- ✅ Validação de campos vazios
- ✅ Verificação de credenciais no banco de dados

### Comportamento Atual
O sistema exibe uma tela de autenticação que solicita nome de usuário e senha. Ao submeter:
- **Se os campos estiverem vazios**: Exibe mensagem de erro
- **Se as credenciais forem inválidas**: Exibe mensagem de erro
- **Se as credenciais forem válidas**: Fecha a janela de login e encerra (próxima etapa: abrir painel principal)

### Próximos Passos
- [ ] Implementar painel principal após login
- [ ] Cadastro de horas extras
- [ ] Relatórios e consultas
- [ ] Gestão de usuários

---

## 🚀 Como Configurar o Ambiente

### Pré-requisitos

Antes de começar, certifique-se de ter instalado:

1. **Compilador C**
   - [Guia de instalação](https://syntaxpathways.com/set-up-c-development-environment/)

2. **IDE Code::Blocks 25.03**
   - [Download aqui](https://sourceforge.net/projects/codeblocks/files/Binaries/25.03/Windows/codeblocks-25.03-setup.exe/download)

3. **MySQL Server**
   - [Download aqui](https://dev.mysql.com/downloads/windows/installer/8.0.html)

4. **MSYS2** (para dependências GTK4)
   - [Download aqui](https://www.msys2.org/)

> ⚠️ **Importante**: Instale todos os programas nos diretórios padrão. Caso contrário, será necessário configurar os caminhos manualmente na IDE.

---

## 📦 Instalação das Dependências

### 1. Instalar GTK4 e Ferramentas

Abra o terminal **MSYS2 MSYS** e execute:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-toolchain
pacman -S base-devel
pacman -S mingw-w64-x86_64-gtk4
pacman -S mingw-w64-x86_64-glade
pacman -Syu
```

### 2. Verificar Instalação

No terminal **MSYS2 MINGW64**, execute:

```bash
pkg-config --cflags gtk4
```

✅ **Sucesso**: O comando retorna flags de compilação  
❌ **Erro**: Repita os passos da seção anterior

---

## 🗄️ Configuração do Banco de Dados

### 1. Criar o Banco de Dados

1. Abra o **MySQL Command Line Client**
2. Faça login com a senha do usuário `root`
3. Execute o arquivo de inicialização:

```sql
source C:\Users\SeuUsuario\caminho\para\o\projeto\init.sql
```

> 📝 **Nota**: Substitua `SeuUsuario` e o caminho pelo local correto do arquivo `init.sql`

### 2. Credenciais Padrão

Após a configuração, use as seguintes credenciais para testar:
- **Usuário**: `admin`
- **Senha**: `1234`

---

## 🔨 Compilação e Execução

### No Code::Blocks

1. Abra o arquivo `Overtime Tracker.cbp` no Code::Blocks
2. Compile o projeto: `Ctrl + F9`
3. Aguarde a mensagem de sucesso na saída
4. Execute o programa: `Ctrl + F10`

### Solução de Problemas

**Erro de compilação relacionado ao GTK4:**
- Verifique se o MSYS2 MINGW64 está no PATH do sistema
- Confirme a instalação do GTK4 com `pkg-config --cflags gtk4`

**Erro de conexão com MySQL:**
- Verifique se o servidor MySQL está rodando
- Confirme as credenciais em `DATABASE_USER` e `DATABASE_PASSWORD` no código
- Certifique-se de que o banco foi criado com o `init.sql`

---

## 📚 Documentação e Referências

### Tutoriais de Configuração
- [Como instalar compilador C](https://syntaxpathways.com/set-up-c-development-environment/)
- [GTK4: Instalação e configuração](https://www.treinaweb.com.br/blog/criando-interfaces-graficas-no-c-com-gtk/)
- [MySQL com C: Configuração](https://www.treinaweb.com.br/blog/utilizando-o-mysql-em-uma-aplicacao-c/)
- [GTK no Code::Blocks](https://www.treinaweb.com.br/blog/criando-uma-aplicacao-c-com-gtk-no-codeblocks/)

### Documentação Oficial
- [GTK4 Documentation](https://docs.gtk.org/gtk4/)
- [MySQL Command Line](https://www.treinaweb.com.br/blog/primeiros-passos-com-mysql)

---

## 👥 Equipe

Projeto Integrador - Programação de Computadores  
Engenharia da Computação - 2º Período

---

## 📝 Licença

Este projeto é desenvolvido para fins acadêmicos.
