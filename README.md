# Overtime-Tracker

# Como construir

## Pré-requisitos
* Um compilador para a linguagem de programação C, se não tiver um instalado, siga [este passo-a-passo](https://syntaxpathways.com/set-up-c-development-environment/);
* A IDE Code-blocks, pode ser obtida [aqui](https://sourceforge.net/projects/codeblocks/files/Binaries/25.03/Windows/codeblocks-25.03-setup.exe/download);
* Um servidor MySQL, pode ser obtido [aqui](https://dev.mysql.com/downloads/windows/installer/8.0.html).
* Experiência prática com IDEs, a linguagem C e com o SGBD do MySQL

OBS.: Para que a compilação funcione, todos os programas mencionados acima devem estar no diretório padrão de instalação, caso contrário, será necessário apontar para as pastas de instalação manualmente apartir da IDE.

## Dependências
Instale todas as dependências da biblioteca GTK4 com o seguinte comando no terminal do MSYS2 MSYS:
```
pacman -Syu && \
pacman -S mingw-w64-x86_64-toolchain && \
pacman -S base-devel && \
pacman -S mingw-w64-x86_64-gtk4 && \
pacman -S mingw-w64-x86_64-glade && \
pacman -Syu
```
Verifique se as depêndencias foram instaladas corretamente executando no terminal do MSYS2 MINGW64:
```
pkg-config --cflags gtk4
```
Se uma mensagem de erro aparecer, refaça os passos acima novamente

## Conexão com o banco de dados
1. Execute o terminal de comando do Cliente do MySQL "MySQL Command Line Client"
2. Autentique-se com sua senha de usuário "root"
3. Execute o arquivo "init.sql" para construir o banco de dados seguindo o comando:
```
source C:\Users\OtavioAugustus\<caminho para o arquivo>\init.sql
```

## Compilação
1. Abra o arquivo "Overtime Tracker.cdp" com a IDE CODE::BLOCKS
2. Pressione Ctrl. + F9 para compilar e aguarde a saída indicar sucesso
3. Pressione Ctrl. + F10 para executar

# Como executar
Baixe o lançamento mais recente [aqui](https://github.com/OtavioAugustusDev/Overtime-Tracker/releases)

# Literatura
Como instalar um compilador para C: https://syntaxpathways.com/set-up-c-development-environment/

Como instalar as dependências necessárias para compilar a biblioteca GTK: https://www.treinaweb.com.br/blog/criando-interfaces-graficas-no-c-com-gtk/

Como configurar uma IDE para trabalhar com MySQL e GTK:
* https://www.treinaweb.com.br/blog/utilizando-o-mysql-em-uma-aplicacao-c/
* https://www.treinaweb.com.br/blog/criando-uma-aplicacao-c-com-gtk-no-codeblocks/

Como construir janelas com GTK4: https://docs.gtk.org/gtk4/

Como utilizar a linha de comando do MySQL: https://www.treinaweb.com.br/blog/primeiros-passos-com-mysql
