/* $Id$ */
#include "bbs.h"

#define NEWSDIRECT  BBSHOME "/boards/n/newspaper"
#define MOVIEDIRECT BBSHOME "/etc/NEWS"

int main(int argc, char **argv)
{
    int fd;
    fileheader_t fh, news;
    struct stat st;
    register int numfiles, n;
    FILE *fp = NULL;
    char buf[200];

    if (stat(NEWSDIRECT "/.DIR", &st) < 0)
	return 0;

    system("rm -f " MOVIEDIRECT "/*");
    system("rm -f " MOVIEDIRECT "/.DIR");

    numfiles = st.st_size / sizeof(fileheader_t);
    n = 0;
    if ((fd = open(NEWSDIRECT "/.DIR", O_RDONLY)) > 0)
    {
	lseek(fd, st.st_size - sizeof(fileheader_t), SEEK_SET);
	while (numfiles-- && n < 100)
	{
	    read(fd, &fh, sizeof(fileheader_t));
	    if (!strcmp(fh.owner, "CNA-News."))
	    {
		if (!strstr(fh.title, "活動預告") && !strstr(fh.title, "中央氣象局")
		    && !strstr(fh.title, "歷史上的今天") && !strstr(fh.title, "頭條新聞標題")
		    && !strstr(fh.title, "Summary") && !strstr(fh.title, "全球氣象一覽")
		    && !strstr(fh.title, "校正公電"))
		{
		    if (!(n % 10))
		    {
			if (n)
			{
			    fclose(fp);
			    append_record(MOVIEDIRECT "/.DIR", &news, sizeof(news));
			}
			strcpy(buf, MOVIEDIRECT);
			stampfile(buf, &news);
			sprintf(news.title, "中央社即時新聞 %s", fh.date);
			strcpy(news.owner, "CNA-News.");
			if (!(fp = fopen(buf, "w")))
			    return (0);
			fprintf(fp, "[34m        ─────────[47;30m 中央社即時新聞[31m (%s)[m[34m──────────\n",
				fh.date);
		    }
		    fprintf(fp, "        ─────[33m◇[m [1;3%dm%s  [m[34m%.*s\n",
			    (n % 6 + 4) % 7 + 1, fh.title,
			    (int)(46 - strlen(fh.title)),
			    "───────────────────");
		    n++;
		    printf("[%d]\n", n);

		}
	    }
	    lseek(fd, -(off_t) 2 * sizeof(fileheader_t), SEEK_CUR);
	}
	close(fd);
	fclose(fp);
	append_record(MOVIEDIRECT "/.DIR", &news, sizeof(news));
    }
    return 0;
}
