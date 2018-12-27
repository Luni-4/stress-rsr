% This implementation is not mine, the comments are in Italian.
% Not fundamental to understand the algorithm, just a reference.

function rsr_stress
% esegue sia STRESS che RSR, con gli stessi parametri
% dato che stress è quasi identico, sfruttiamo i calcoli per filtrare
% con entrambi i metodi
% permette di aprire più file
% permette anche di ridimensionare l'img

% trova il file da aprire
[file,path]=uigetfile({'*.*',  'All Files (*.*)'},'MultiSelect','on');
filename=strcat(path, file);

% devo capire se ho aperto uno o più file
% in quest ultimo i caso i file sono messe in celle
if (iscell(file))
    % numero di file aperti
    n_file=length(file);
else
    n_file=1;
end

Nit=input('Inserisci il numero di iterazioni -> ');
disp(' ');
Ns=input('Inserisci il numero di campioni -> ');
disp(' ');
disp('Inserisci il fattore di divisione della diagonale');
disp('per il calcolo del raggio dello spray. Esempi: ');
disp('1 = diagonale img')
disp('2 = metà della diagonale img (effetto più locale)')
fattore_dist=input('-> ');
disp(' ');
disp('Inserisci il fattore di scala della img');
f_scala=input('(1: originale, 2: dimezza, 3: un terzo ecc.) -> ');
disp(' ');
disp('***');
disp(' ');

for n_f=1:n_file
    tic

    if (n_file==1)
        img_letta=imread(filename);
        img_letta=double(img_letta);
        nome_file=filename;
    else
        img_letta = imread(filename{n_f});
        img_letta=double(img_letta);
        % nome del file
        nome_file=filename{n_f};
    end
    disp(nome_file); % visualizziamo il nome per sapere quale sto facendo

    if (f_scala~=1)
        img_letta=imresize(img_letta, 1/f_scala); 
        % attenzione che nel fare il resize magari ho dei numeri negativi
        % non è chiaro perchè... quindi li rimetto positivi
        img_letta=max(img_letta,0);
    end

    % calcolo le dimensioni
    [r,c,p]=size(img_letta);

    % min_lato sarebbe la distanza entro la quale lancio gli spray
    min_lato=sqrt(r^2+c^2)/fattore_dist;

    % riempiamo questa matrice che sarà poi l'img filtrata
    img_filtrata_stress=zeros(r,c,p);
    img_filtrata_rsr=zeros(r,c,p);

    % ciclo su tutta l'img
    for j1=1:r
        for j2=1:c
            % ciclo per il num di iterazioni
            b=[];
            for j3=1:Nit
                % ciclo per il num di campioni
                for j4=1:Ns
                    % campionamento radiale, serie di controlli
                    %[j1 j2 j3 j4]
                    % devo scegliere i pixel a caso: riga e colonna
                    % scelgo la distanza, tra 0 e R/2, e l'angolo tra o e 2
                    % pigreco
                    dist=rand()*min_lato;
                    theta=rand()*360;
                    % quindi con le trasformazioni di coordinate polari calcolo
                    %   l'incremento su x e su y
                    inc_x=dist*cos((theta*pi/180));
                    inc_y=dist*sin((theta*pi/180));
                    % la riga sara data dalla posizione attuale piu
                    % l'incremento. Faccio l'arrotondamento a intero e anche il
                    % valore assoluto, cioe nel caso esco dall'immagine mi
                    % riporto dentro
                    riga=round(abs(j1+inc_x));
                    colonna=round(abs(j2+inc_y));               
                    % se ho preso un punto che non è nell'img, quel punto
                    % lo sostituisco con un punto a caso nell'img
                    if (riga<1 | riga>r | colonna<1 | colonna>c)
                        % devo scegliere i pixel a caso: riga e colonna
                        %disp('entrato')
                        riga=ceil(rand()*r);
                        colonna=ceil(rand()*c);
                    end % end IF
                    % metto in un vettore i campioni
                    samples_vector(j4,1)=img_letta(riga,colonna,1);
                    samples_vector(j4,2)=img_letta(riga,colonna,2);
                    samples_vector(j4,3)=img_letta(riga,colonna,3);                
                 end % end j4
                 %samples_vector
                 % il sample vector deve contenere anche il pixel che sto
                 % considerando
                 samples_vector(Ns+1,1)=img_letta(j1,j2,1);
                 samples_vector(Ns+1,2)=img_letta(j1,j2,2); 
                 samples_vector(Ns+1,3)=img_letta(j1,j2,3);             

                 s_max(1)=max(samples_vector(:,1));
                 s_min(1)=min(samples_vector(:,1));
                 r_value(1)=s_max(1)-s_min(1);  

                 if (r_value(1)==0)
                      b(j3,1)=0.5;
                   else
                      b(j3,1)=(img_letta(j1,j2,1)-s_min(1))/r_value(1);
                 end

                 s_max(2)=max(samples_vector(:,2));
                 s_min(2)=min(samples_vector(:,2));
                 r_value(2)=s_max(2)-s_min(2);

                 if (r_value(2)==0)
                      b(j3,2)=0.5;
                   else
                      b(j3,2)=(img_letta(j1,j2,2)-s_min(2))/r_value(2);
                 end

                 s_max(3)=max(samples_vector(:,3));
                 s_min(3)=min(samples_vector(:,3));
                 r_value(3)=s_max(3)-s_min(3);

                 if (r_value(3)==0)
                      b(j3,3)=0.5;
                   else
                      b(j3,3)=(img_letta(j1,j2,3)-s_min(3))/r_value(3);
                 end
                 
               

                 % parte RSR       
                 b_rsr(j3,1)=img_letta(j1,j2,1)/s_max(1);
                 b_rsr(j3,2)=img_letta(j1,j2,2)/s_max(2);
                 b_rsr(j3,3)=img_letta(j1,j2,3)/s_max(3);

                 %b
               %pause
            end % end j3
            % faccio la media delle iterazioni, per i tre canali separatmante
            img_filtrata_stress(j1,j2,1)=mean(b(:,1));
            img_filtrata_stress(j1,j2,2)=mean(b(:,2));
            img_filtrata_stress(j1,j2,3)=mean(b(:,3));

            img_filtrata_rsr(j1,j2,1)=mean(b_rsr(:,1));
            img_filtrata_rsr(j1,j2,2)=mean(b_rsr(:,2));
            img_filtrata_rsr(j1,j2,3)=mean(b_rsr(:,3));
        end % end j2

        % mostra la percentuale ogni 5%
        % attenzione, forse per ottimizzare, non sempre visualizza
        % sopratutto se i calcoli sono pesanti
        % per essere sicuri di visulizzare qualcosa, si può semplicemnte
        % mettere "j1"
        perc_r=j1/r*100;
        if (mod(perc_r,5)==0) 
            disp(perc_r)
        end
    end % end j1

    % faccio il mapping delle img
    %img_filtrata_stress=img_mapping(img_filtrata_stress);
    %img_filtrata_rsr=img_mapping(img_filtrata_rsr);
    
    img_filtrata_stress=min(img_filtrata_stress,1);
    img_filtrata_stress=max(img_filtrata_stress,0);
    
    
    img_filtrata_rsr=min(img_filtrata_rsr,1);
    img_filtrata_rsr=max(img_filtrata_rsr,0);

    % visualizzo l'img
    figure()
    imshow(img_filtrata_stress)
    title('stress')
    figure()
    imshow(img_filtrata_rsr)
    title('rsr')

    % salvo l'img
    nome_stress=strcat(nome_file,'_stress_Nit_',int2str(Nit),'_Ns_',int2str(Ns),'.tiff');
    imwrite(img_filtrata_stress,nome_stress)

    nome_rsr=strcat(nome_file,'_rsr_Nit_',int2str(Nit),'_Ns_',int2str(Ns),'.tiff');
    imwrite(img_filtrata_rsr,nome_rsr)
    toc
    disp('***')
end % fine ciclo sui vari file, se selezionati multipli

