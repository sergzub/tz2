Примечания к реализации задания (см. файл tz.txt).

В состав менеджера, помимо собственно блоков оперативной памяти пула (std::vector<char> blocks_;),
входят дополнительные служебные структуры (std::vector<BlockExt> blocksExt_;), которые позволяют вполне
эффективно представлять и модифицировать текущее состояние элементов менеджера (AllocatorImpl).
В частоности, в каждом из пулов (16, 32 ...) каджый блоки представлен в одном из двух списков:
    - однонаправленный список свободных блоков (availHeadIdx_ -- указывает на его голову);
    - двунапрвленный список для распределенных в текущий момент блоков пула (headAllocIdx_ + tailAllocIdx_)
Вместо указателей, в реализации списков применяется номер/индекс блока в пуле (BlockIdx).

При запросе клиентом блока менеджер определяет, есть ли сободные блоки в соответствующем пулие.
Если да, то помещает его в список распределенных, записывает туда клиентские данные и возвращает адрес блока клиенту.
Адрес блока (struct BlockAddress) составной, и включает в себя:
    - длина полезной области в блоке (len_);
    - номер/индекс блока в пуле (blkIdx_), который однозначно хотя и определяется одначно по len_;
      но в дальнейшем используется для последующего восставноления данных из блока;
    - ид распределнного блока (blkId_), уникален в пределах пула (формируется на базе инкрементального счетчика обращений к пулу),
      нужен для определения находится ли блок менеджера в памяти или был сброшен в swap, а также позволяет адресовать его в файле swap
Если доступной свободной памяти в менеджере нет, то:
    - берется самый старый рапределенный блок (голова списка занятых);
    - его содержимое сбрасывается в swap;
    - блок освобождается и тут же распределяется с уже новым содержимым (при этом делает его самым новым распределенным блоком);

Хранилище swap -- набор файлов (в каталоге ../swap), где каждый файл соответствует отдельному пулу.
При этом блок в таком файле сохраняется/восстанавливается по смещению, равному значению blkId_ в его адресе.

При освобожднении блока менеждер определяет где находится блок (в памяти или swap) и восстаналивает его содержимое в клиенскую область памяти.
В данной реализации допущено упрощение, которое не предполагает двухфазной работы с менадежером:
    1. запрос реального адреса блока (с возможной педварительной подкачкой его в память блока пула -- свободного или занятого)
    2. освобождение блока
Сделать это не представляется сильно сложным, однако в рамках даного задания кажется избыточным.

Сборка и подготовка исходных файлов выполяется скриптом build.sh.
 
Запуск исполняемого файла (tz2) нужно выполнять из его каталога: 
 $ ./tz2 100

 Первый параметр задает размер оперативного памяти менеджера памяти, выделяемого под пулы.
 Задается в MB.

Входные  файлы: ../in
Выходные файлы: ../out
(относительно исполняемого файла в build/tz2)
