import os

os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'indexer_proj.settings')

import django

django.setup()

from indexer_app.models import Category, Page

indexes = {
    'Python': [
        {
            'title': 'Official Python Tutorial',
            'url': 'https://docs.python.org/3/tutorial/'
        },
        {
            'title': 'Official Django Tutorial',
            'url': 'https://docs.djangoproject.com/',
        },
    ],
    'Search': [
        {
            'title': 'Baidu Search Engine',
            'url': 'https://www.baidu.com/',
        },
        {
            'title': 'Bing Search Engine',
            'url': 'https://www.bing.com/',
        },
        {
            'title': 'Google Search Engine',
            'url': 'https://www.google.com/',
        },
    ]
}


def populate():
    for cat, pages in indexes.items():
        c = Category.objects.get_or_create(name=cat)[0]
        c.save()
        for page in pages:
            p, _ = Page.objects.update_or_create(category=c,
                                                 title=page['title'],
                                                 defaults={'url': page['url']})
            print(f'{c} - {p}')


if __name__ == '__main__':
    populate()
