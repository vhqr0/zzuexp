from django.contrib import admin

from .models import Category, Page


class PageInline(admin.StackedInline):
    model = Page


class CategoryAdmin(admin.ModelAdmin):
    inlines = [PageInline]


class PageAdmin(admin.ModelAdmin):
    list_display = ('title', 'category', 'url')
    list_filter = ['category']
    search_fields = ['title']


admin.site.register(Category, CategoryAdmin)
admin.site.register(Page, PageAdmin)
