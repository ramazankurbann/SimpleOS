-Tasarımda CPU,Ram, işletim sistemi, kullanıcı programları simüle edilmeye çalışıldı.
Hangi birim simüle edildiyse fonksiyon veya değişken ismi önüne o birimin eki koyuldu.

-Bir bilgisayarda tek CPU olduğu için multithread yapılmadı. Main içinde işletim sistemi
kernel'ı benzeri bir tasarım yapılıp her bir process bu kernel tarafından tek bir CPU
kaynağı üzerinde Round Robin ile sırayla çalıştırıldı. Bu şekilde paralellik sağlandı.

-Kişisel tercih olarak Hardvard mimarisi kullandım. tasks.txt içindeki erişim sağlanacak
adressleri load instruction'ın immediate değerleri olarak kullanıldı ve bir Program Memory'si
oluşturuldu.A.txt benzeri veri tutulduğu yer için ayrı bir Data Memory kullanıldı.Bu memory'leri
int array'leri şeklinde kod içinde simüle edildi.

-Process state machine'ı kullanıldı. task.txt içinden okunan process'ler New Queue'ya atıldı.
Admitter bu queue'dan process'leri okuyup uygunsa Ready queue'ya atar. Dispatcher ready queueu'dan
process seçer ve context swithcer'a verip CPU context'i değiştirilir. Sonrasında CPU executor fonksiyonu
çağırılır ve cpu çalışması simüle edilir. CPU içinde kullanıcı programı hardcoded olarak eklenmiştir ama
parametresi Program Memory'den program counter ile seçilir.Round robin için load instruction'ı içinde
cpu timer değişkeni arttırılır ve zamanın geçmesi simüle edilir.Timer interrupt'ı hardcoded olarak kontrol
edilir. Interrupt oluşunca process ready queue'ya atılır. Bu tekrarlanır.

-Page'lerin oluşturulması için free space management yapıldı. Memory'de ki herbir boş bölüm bloklar halinde
array'de tutuldu. Bir process' geldiğinde bu array'den boş bir blok seçilir ve blok içinden frame'ler verilir.
Memory dolduğunda arada boşluklu bir yapı oluştuğunda rastgele page dağılı yapılabilecek bir algoritmaya sahiptir.
Başlangıçta memory boş olduğu için uygun olan blok'tan page'ler verilir ama rastgele kullanımada uygundur.

-Memory management unit simülasyonu yapıldı. Adresler limitlere uygunmu kontrol edildi. TLB'de bulunmayan page'ler
ram içindeki belirli bölgelerden çekilip TLB içine atıldı.

-a.txt içinde ne olduğu ile ilgili bilgi verilmediği için o değerler dummy olarak kullanıldı. Sadece erişim
sağlanabilir mi onunla ilgilenildi.






